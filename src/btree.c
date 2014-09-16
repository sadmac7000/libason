/**
 * Copyright © 2014 Red Hat, Casey Dahlin <casey.dahlin@gmail.com>
 *
 * This file is part of libason.
 *
 * libason is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libason is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libason. If not, see <http://www.gnu.org/licenses/>.
 **/

#include <endian.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <err.h>

#include <sys/types.h>
#include <sys/mman.h>

#include "btree.h"
#include "crc.h"
#include "util.h"

/**
 * Journal commands.
 **/
typedef char journal_cmd_t;
#define JOURNAL_COMMIT         0x0
#define JOURNAL_SET_ROOT       0x1
#define JOURNAL_BLOCK_SUBTRACT 0x2
#define JOURNAL_ADD_KV         0x3
#define JOURNAL_BLOCK_MERGE    0x4
#define JOURNAL_DELETE         0x5

#define JOURNAL_START 16

/**
 * A pairing of a block and a mapping.
 **/
struct block_mapping {
	block_t block;
	void *mapping;
};

/**
 * A B-Tree descriptor.
 **/
struct btree {
	blockfile_t *blockfile;
	void *root_map;
	size_t journal_pos;
};

/**
 * Allocate a new empty B-tree node. If `map` is not NULL, the block will be
 * mapped there.
 **/
static ssize_t
btree_allocate_new_block(btree_t *btree, uint64_t **map)
{
	ssize_t ret = blockfile_allocate(btree->blockfile, 1);
	uint64_t *mapping;
	size_t i;

	if (ret < 0)
		return ret;

	mapping = blockfile_map(btree->blockfile, ret);

	if (! mapping) {
		blockfile_free(btree->blockfile, ret);
		return -ENOMEM;
	}

	for (i = 0; i < BLOCK_SIZE / 8; i++)
		mapping[i] = htobe64(BLOCK_BAD_MISSING);

	if (map)
		*map = mapping;
	else
		blockfile_unmap(btree->blockfile, mapping);

	return ret;
}

/**
 * Write some data to the journal. Allow use of the reserved trailer space.
 **/
static void
btree_journal_write_final(btree_t *btree, const char *data, size_t len)
{
	memcpy(btree->root_map + btree->journal_pos, data, len);

	btree->journal_pos += len;
	return;
}

/**
 * Write some data to the journal
 **/
static int
btree_journal_write(btree_t *btree, const char *data, size_t len)
{
	if (btree->journal_pos + len > BLOCK_SIZE - 9)
		return -ENOSPC;

	btree_journal_write_final(btree, data, len);
	return 0;
}

/**
 * Remove all of the keys in `other` from `block`.
 **/
static void
btree_do_block_subtract(btree_t *btree, uint64_t block_be, uint64_t other_be)
{
	uint64_t *block_words = blockfile_map(btree->blockfile,
					      be64toh(block_be));
	uint64_t *other_words = blockfile_map(btree->blockfile,
					      be64toh(other_be));
	size_t items = BLOCK_SIZE / 16;
	size_t i, j;

	for (i = 0; i < items; i++) {
		if (other_words[i * 2 + 1] == htobe64(BLOCK_BAD_MISSING))
			continue;

		for (j = 0; j < items; j++) {
			/* Big-endian comparison, but equality shouldn't care
			 */
			if (block_words[j * 2] != other_words[i * 2])
				continue;

			block_words[j * 2 + 1] = htobe64(BLOCK_BAD_MISSING);
			break;
		}
	}

	blockfile_unmap(btree->blockfile, other_words);
	msync(block_words, BLOCK_SIZE, MS_SYNC);
	blockfile_unmap(btree->blockfile, block_words);
}

/**
 * Set the root of the B-Tree.
 **/
static void
btree_do_set_root(btree_t *btree, uint64_t root_be, uint64_t depth_be)
{
	uint32_t *depth_pos = btree->root_map + 4;
	uint64_t *root_pos = btree->root_map + 8;

	*root_pos = root_be;

	*depth_pos = htobe32((uint32_t)be64toh(depth_be));

	msync(btree->root_map, BLOCK_SIZE, MS_SYNC);
}

/**
 * Add a key-value pair to a B-tree
 **/
static void
btree_do_add_kv(btree_t *btree, uint64_t block_be, uint64_t key_be,
		uint64_t value_be)
{
	uint64_t *block = blockfile_map(btree->blockfile, be64toh(block_be));
	size_t i;

	for (i = 1; i < BLOCK_SIZE / 8; i += 2)
		if (block[i] == htobe64(BLOCK_BAD_MISSING))
			break;

	block[i] = value_be;
	block[i - 1] = key_be;

	msync(block, BLOCK_SIZE, MS_SYNC);
	blockfile_unmap(btree->blockfile, block);
}

/**
 * Delete a key-value pair from a block
 **/
static void
btree_do_delete(btree_t *btree, uint64_t block_be, uint64_t key_be)
{
	uint64_t *block = blockfile_map(btree->blockfile, be64toh(block_be));
	size_t i;

	for (i = 0; i < BLOCK_SIZE / 8; i += 2) {
		if (block[i] != key_be)
			continue;
		if (block[i + 1] == htobe64(BLOCK_BAD_MISSING))
			continue;
	}

	block[i + 1] = htobe64(BLOCK_BAD_MISSING);

	msync(block, BLOCK_SIZE, MS_SYNC);
	blockfile_unmap(btree->blockfile, block);
}

/**
 * Merge a block into another.
 **/
static void
btree_do_block_merge(btree_t *btree, uint64_t block_be, uint64_t other_be)
{
	uint64_t *block = blockfile_map(btree->blockfile, be64toh(block_be));
	uint64_t *other = blockfile_map(btree->blockfile, be64toh(other_be));
	size_t i = 1;
	size_t j = 1;

	for (;;) {
		for (; i < BLOCK_SIZE / 8; i += 2)
			if (block[i] == htobe64(BLOCK_BAD_MISSING))
				break;

		for (; j < BLOCK_SIZE / 8; i += 2)
			if (other[j] != htobe64(BLOCK_BAD_MISSING))
				break;

		if (i == BLOCK_SIZE / 8)
			break;

		if (j == BLOCK_SIZE / 8)
			break;

		block[i] = other[j];
		block[i - 1] = other[j - 1];

		/* FIXME: If we replay this action twice, due to recovering the
		 * journal, we'll get duplicate keys. Fix would require
		 * forbidding duplicate keys in general and checking for them
		 * here and elsewhere.
		 */
	}

	msync(block, BLOCK_SIZE, MS_SYNC);
	blockfile_unmap(btree->blockfile, block);
	blockfile_unmap(btree->blockfile, other);
}

/**
 * Execute a single journal instruction.
 **/
static void
btree_do_journal_inst(btree_t *btree, char inst, uint64_t args[3])
{
	switch (inst) {
	case JOURNAL_SET_ROOT:
		btree_do_set_root(btree, args[0], args[1]);
		break;
	case JOURNAL_BLOCK_SUBTRACT:
		btree_do_block_subtract(btree, args[0], args[1]);
		break;
	case JOURNAL_ADD_KV:
		btree_do_add_kv(btree, args[0], args[1], args[2]);
		break;
	case JOURNAL_BLOCK_MERGE:
		btree_do_block_merge(btree, args[0], args[1]);
		break;
	case JOURNAL_DELETE:
		btree_do_delete(btree, args[0], args[1]);
		break;
	default:
		errx(1, "Corrupt journal passed checksum");
	};
}

/**
 * Execute a single journal action at the given position, and return the
 * expected position of the next action.
 **/
static size_t
btree_journal_execute_one(btree_t *btree, size_t pos)
{
	char inst = ((char *)btree->root_map)[pos];
	uint64_t unpack[3];
	size_t args = 2;

	if (! inst)
		return pos;

	pos++;

	if (inst == JOURNAL_SET_ROOT)
		args = 1;

	if (inst == JOURNAL_ADD_KV)
		args = 3;

	memcpy(unpack, btree->root_map + pos, 8 * args);

	pos += 8 * args;

	btree_do_journal_inst(btree, inst, unpack);

	return pos;
}

/**
 * Abort the journal.
 **/
static void
btree_journal_abort(btree_t *btree)
{
	char *journal = btree->root_map;
	uint64_t checksum = htobe64(crc64((unsigned char *)"\0", 1));

	btree->journal_pos = JOURNAL_START;

	journal[JOURNAL_START] = '\0';

	memcpy(journal + JOURNAL_START + 1, &checksum, 8);
}

/**
 * Execute the contents of the journal.
 **/
static void
btree_journal_execute(btree_t *btree)
{
	size_t pos = JOURNAL_START;
	char *journal = btree->root_map;

	while (journal[pos])
		pos = btree_journal_execute_one(btree, pos);

	btree_journal_abort(btree);
}

/**
 * Commit the journal.
 **/
static void
btree_journal_commit(btree_t *btree)
{
	uint64_t checksum;

	btree_journal_write_final(btree, "\0", 1);
	checksum = crc64(btree->root_map + JOURNAL_START,
			 btree->journal_pos - JOURNAL_START);
	checksum = htobe64(checksum);
	btree_journal_write_final(btree, (char *)&checksum, 8);

	msync(btree->root_map, BLOCK_SIZE, MS_SYNC);

	btree_journal_execute(btree);
}

/**
 * Write an instruction to the journal.
 **/
static int
btree_journal_write_inst(btree_t *btree, char inst, ...)
{
	size_t arg_count = 2;
	size_t i;
	va_list ap;

	if (inst == JOURNAL_ADD_KV)
		arg_count = 3;
	if (inst == JOURNAL_DELETE)
		arg_count = 1;

	char data[8 * arg_count + 1];
	uint64_t *words = (uint64_t *)(data + 1);

	data[0] = inst;

	va_start(ap, inst);
	for (i = 0; i < arg_count; i++)
		words[i] = htobe64(va_arg(ap, uint64_t));
	va_end(ap);

	return btree_journal_write(btree, data, 17);
}

/**
 * Find the block that should house `key`. Return the chain of blocks that mark
 * its parent. If height_ret is set, it becomes the length of the returned
 * array.
 **/
static uint64_t *
btree_lookup_block(btree_t *btree, uint64_t key, size_t *height_ret)
{
	uint32_t height;
	block_t block;
	size_t depth = 0;
	uint64_t *words;
	size_t i;
	size_t best;
	uint64_t *ret;

	height = be32toh(*(uint32_t *)(btree->root_map + 4));
	block = be64toh(*(uint64_t *)(btree->root_map + 8));

	if (height_ret)
		*height_ret = height;

	if (block == BLOCK_BAD_MISSING)
		return NULL;

	ret = xcalloc(height, sizeof(uint64_t));

walk_down:
	ret[depth] = block;

	if (height == depth)
		return ret;

	words = blockfile_map(btree->blockfile, block);

	best = BLOCK_SIZE;
	for (i = 0; i < BLOCK_SIZE / 8; i += 2) {
		if (be64toh(words[i + 1]) == BLOCK_BAD_MISSING)
			continue;

		if (best == BLOCK_SIZE)
			best = i;

		if (be64toh(words[i]) > key)
			continue;

		if (be64toh(words[i]) < be64toh(words[best]))
			continue;

		best = i;
	}

	block = be64toh(words[best]);
	depth++;
	blockfile_unmap(btree->blockfile, words);

	goto walk_down;
}

/**
 * Create a new B-tree within a blockfile.
 **/
btree_t *
btree_create(blockfile_t *blockfile, uint32_t flags)
{
	btree_t *ret;
	ssize_t root = blockfile_allocate(blockfile, 1);
	void *root_map;

	if (root < 0)
		return NULL;

	root_map = blockfile_map(blockfile, root);

	if (! root_map) {
		blockfile_free(blockfile, root);
		return NULL;
	}

	*(uint32_t *)root_map = htobe32(flags);
	*(uint32_t *)(root_map + 4) = htobe32(0); /* Tree depth */
	*(uint64_t *)(root_map + 8) = htobe64(BLOCK_BAD_MISSING);
	memset(root_map + JOURNAL_START, 0, BLOCK_SIZE - JOURNAL_START);
	blockfile_sync(blockfile, root_map, 0, 1, 1);

	ret = xcalloc(1, sizeof(btree_t));
	ret->blockfile = blockfile_get_ref(blockfile);
	ret->root_map = root_map;
	ret->journal_pos = JOURNAL_START;

	return ret;
}

/**
 * Load a B-Tree given the block number of its head block.
 **/
btree_t *
btree_load_headblock(blockfile_t *blockfile, size_t block_num)
{
	btree_t *ret;
	void *root_map;

	root_map = blockfile_map(blockfile, block_num);

	if (! root_map)
		return NULL;

	ret = xcalloc(1, sizeof(btree_t));
	ret->blockfile = blockfile_get_ref(blockfile);
	ret->root_map = root_map;
	ret->journal_pos = JOURNAL_START;

	return ret;
}

/**
 * Load a B-Tree given the annotation which marks its head block.
 **/
btree_t *
btree_load_annotation(blockfile_t *blockfile, const char *annotation)
{
	block_t block = blockfile_get_annotated_block(blockfile, annotation);

	if (block >= BLOCK_ERROR_START)
		return NULL;

	return btree_load_headblock(blockfile, (size_t)block);
}

/**
 * Insert a value into an empty B-tree. Returns 0 on success.
 **/
static int
btree_insert_first(btree_t *btree, uint64_t key, uint64_t data)
{
	ssize_t block;
	uint64_t *items;
	int got;

	block = btree_allocate_new_block(btree, &items);

	if (block < 0)
		return (int)block;

	items[0] = htobe64(key);
	items[1] = htobe64(data);

	msync(items, BLOCK_SIZE, MS_SYNC);
	blockfile_unmap(btree->blockfile, items);

	got = btree_journal_write_inst(btree, JOURNAL_SET_ROOT, block);

	if (! got) {
		btree_journal_commit(btree);
		return 0;
	}

	blockfile_free(btree->blockfile, block);
	btree_journal_abort(btree);
	return got;
}

/**
 * Create a new node that splits an existing node.
 **/
static int
btree_journal_setup_split(btree_t *btree, uint64_t node,
			  block_t *new_block, uint64_t *split_key)
{
	uint64_t *page;
	uint64_t *new_page;
	ssize_t got;
	uint64_t global_min = 0;
	uint64_t pass_min = 0xffffffffffffffff;
	size_t i;
	size_t pos;
	size_t pass_min_pos = 0;

	page = blockfile_map(btree->blockfile, node);

	if (! page)
		return -ENOMEM;

	got = btree_allocate_new_block(btree, &new_page);

	if (got < 0) {
		blockfile_unmap(btree->blockfile, page);
		return -ENOSPC;
	}

	*new_block = (uint64_t)got;

	/* While new_block is less than half-full */
	for (pos = 0; pos < BLOCK_SIZE / 8 / 2; pos += 2) {
		for (i = 0; i < BLOCK_SIZE / 8; i += 2) {
			if (be64toh(page[i + 1]) == BLOCK_BAD_MISSING)
				continue;

			if (be64toh(page[i]) < global_min)
				continue;

			if (be64toh(page[i] > pass_min))
				continue;

			pass_min = be64toh(page[i]) + 1;
			pass_min_pos = i;
		}

		global_min = pass_min;

		new_page[pos] = page[pass_min_pos];
		new_page[pos + 1] = page[pass_min_pos + 1];
	}

	msync(new_block, BLOCK_SIZE, MS_SYNC);
	blockfile_unmap(btree->blockfile, new_page);
	*split_key = be64toh(page[pass_min_pos]);

	blockfile_unmap(btree->blockfile, page);
	return 0;
}

/**
 * Insert a value into a B-tree. Returns 0 on success.
 **/
int
btree_insert(btree_t *btree, uint64_t key, uint64_t data)
{
	size_t depth;
	uint64_t *path = btree_lookup_block(btree, key, &depth);
	uint64_t *page = NULL;
	block_t new_block;
	ssize_t new_root;
	uint64_t split_key;
	uint64_t root_key = 0xffffffffffffffff;
	uint64_t root_block;
	int got = 0;
	size_t i;

	if (! path)
		return btree_insert_first(btree, key, data);

	for (; depth; depth--) {
		page = blockfile_map(btree->blockfile, path[depth - 1]);

		for (i = 1; i < BLOCK_SIZE / 8; i += 2)
			if (page[i] == htobe64(BLOCK_BAD_MISSING))
				break;

		if (i >= BLOCK_SIZE) {
			got = btree_journal_write_inst(btree, JOURNAL_ADD_KV,
						       path[depth - 1], key,
						       data);

			new_block = BLOCK_BAD_MISSING;
			break;
		}

		got = btree_journal_setup_split(btree, path[depth - 1],
						&new_block, &split_key);

		if (got)
			break;

		if (split_key > key)
			got = btree_journal_write_inst(btree, JOURNAL_ADD_KV,
						       path[depth - 1], key,
						       data);
		else
			got = btree_journal_write_inst(btree, JOURNAL_ADD_KV,
						       new_block, key,
						       data);

		if (got)
			break;

		key = split_key;
		data = new_block;
		blockfile_unmap(btree->blockfile, page);
		page = NULL;

	}

	free(path);

	if (page)
		blockfile_unmap(btree->blockfile, page);

	if (got) {
		btree_journal_abort(btree);
		return got;
	}

	if (new_block == BLOCK_BAD_MISSING) {
		btree_journal_commit(btree);
		return 0;
	}

	root_block = be64toh(*(uint64_t *)(btree->root_map + 8));
	page = blockfile_map(btree->blockfile, root_block);

	if (! page) {
		btree_journal_abort(btree);
		return -ENOMEM;
	}

	for (i = 1; i < BLOCK_SIZE / 8; i += 2) {
		if (page[i] == htobe64(BLOCK_BAD_MISSING))
			continue;

		if (htobe64(page[i - 1]) >= root_key)
			continue;

		root_key = htobe64(page[i - 1]);
	}

	blockfile_unmap(btree->blockfile, page);

	new_root = btree_allocate_new_block(btree, &page);

	if (new_root < 0) {
		btree_journal_abort(btree);
		return (int)new_root;
	}

	page[0] = htobe64(split_key);
	page[1] = htobe64(new_block);
	page[2] = htobe64(root_key);
	page[3] = htobe64(root_block);

	blockfile_unmap(btree->blockfile, page);

	got = btree_journal_write_inst(btree, JOURNAL_SET_ROOT, new_root);

	if (got)
		btree_journal_abort(btree);
	else
		btree_journal_commit(btree);

	return got;
}

/**
 * Return whether a block is empty.
 **/
static int
btree_block_empty(btree_t *btree, block_t block)
{
	uint64_t *map = blockfile_map(btree->blockfile, block);
	size_t i;

	/* FIXME: We need to propagate blockfile map errors */
	if (! map)
		errx(1, "Critical block mapping failure");

	for (i = 0; i < BLOCK_SIZE / 8; i++)
		if (map[i + 1] != htobe64(BLOCK_BAD_MISSING))
			return 0;

	blockfile_unmap(btree->blockfile, map);

	return 1;
}

/**
 * Return the minimum key in a B-tree block.
 **/
static uint64_t
btree_min_key(btree_t *btree, block_t block)
{
	uint64_t *map = blockfile_map(btree->blockfile, block);
	uint64_t ret = 0xffffffffffffffff;
	size_t i;

	/* FIXME: We need to propagate blockfile map errors */
	if (! map)
		errx(1, "Critical block mapping failure");

	for (i = 0; i < BLOCK_SIZE / 8; i++) {
		if (map[i + 1] == htobe64(BLOCK_BAD_MISSING))
			continue;

		if (htobe64(map[i]) < ret)
			ret = htobe64(map[i]);
	}

	blockfile_unmap(btree->blockfile, map);

	return ret;
}

/**
 * Delete an element from a B-Tree.
 **/
int
btree_delete(btree_t *btree, uint64_t key)
{
	size_t depth;
	size_t max_depth;
	uint64_t *path = btree_lookup_block(btree, key, &depth);

	max_depth = depth;

	do {
		depth--;

		if (btree_journal_write_inst(btree, JOURNAL_DELETE,
					     path[depth], key))
			goto fail;

		key = btree_min_key(btree, path[depth]);
	} while (btree_block_empty(btree, path[depth]) && depth);

	if (depth == 0 && btree_block_empty(btree, path[0]))
		goto commit;

	if (btree_journal_write_inst(btree, JOURNAL_SET_ROOT,
				     BLOCK_BAD_MISSING))
		goto fail;

commit:
	btree_journal_commit(btree);

	while (depth < max_depth - 1)
		blockfile_free(btree->blockfile, path[--max_depth]);

	free(path);
	return 0;

fail:
	btree_journal_abort(btree);
	free(path);
	return -ENOSPC;
}

/**
 * Look up a value in a leaf page.
 **/
static uint64_t
btree_leaf_lookup(void *leaf, uint64_t key)
{
	uint64_t *words = leaf;
	uint64_t *words_stop = leaf + BLOCK_SIZE;

	while (words != words_stop && key != be64toh(*words))
		words += 2;

	if (words == words_stop)
		return BLOCK_BAD_MISSING;

	return *(words + 1);
}

/**
 * Look up a value in a B-Tree.
 **/
uint64_t
btree_lookup(btree_t *btree, uint64_t key)
{
	size_t size;
	uint64_t *blocks = btree_lookup_block(btree, key, &size);
	void *page = blockfile_map(btree->blockfile, blocks[size - 1]);
	uint64_t ret;

	free(blocks);

	if (! size)
		return BLOCK_BAD_MISSING;

	ret = btree_leaf_lookup(page, key);

	blockfile_unmap(btree->blockfile, page);
	return ret;
}
