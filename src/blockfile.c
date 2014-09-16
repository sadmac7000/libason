/**
 * Copyright © 2013, 2014 Red Hat, Casey Dahlin <casey.dahlin@gmail.com>
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

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <endian.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "blockfile.h"
#include "util.h"
#include "crc.h"

/**
 * A range of blocks.
 **/
struct block_range {
	block_t offset;
	bfsize_t length;
	void *mem_loc;
	size_t refcount;
};

/**
 * A block file.
 **/
struct blockfile {
	int fd;
	bfsize_t refcount;
	struct block_range *mapped;
	bfsize_t mapped_count;
	void *metapage;
	void **colormaps;
	bfsize_t colormap_count;
};

/**
 * Confirm the magic on a blockfile.
 **/
static int
blockfile_validate(blockfile_t *blockfile)
{
	return !memcmp(blockfile->metapage, BLOCK_MAGIC, BLOCK_MAGIC_LENGTH);
}

/**
 * Initialize the colormap table for a newly-opened blockfile_t
 **/
static int
blockfile_init_colormaps(blockfile_t *blockfile)
{
	off_t end = lseek(blockfile->fd, 0, SEEK_END);
	bfsize_t i;

	if (end < 0)
		err(1, "Could not lseek blockfile");

	end /= BLOCK_SIZE;
	end -= 1;
	end += BLOCK_COLOR_ENTRIES;

	end /= (BLOCK_COLOR_ENTRIES + 1);

	blockfile->colormaps = xcalloc(end, sizeof(void *));

	for (i = 0; i < (bfsize_t)end; i++) {
		blockfile->colormaps[i] = mmap(NULL, BLOCK_SIZE,
					       PROT_READ | PROT_WRITE,
					       MAP_SHARED, blockfile->fd,
					       BLOCK_SIZE +
					       (BLOCK_COLOR_ENTRIES + 1) * i);

		if (blockfile->colormaps[i] == MAP_FAILED)
			return -1;
	}

	blockfile->colormap_count = end;

	return 0;
}

/**
 * Convert block numbers to file offsets in blocks. Block numbers don't count
 * the metadata or the colormap blocks.
 **/
static bfsize_t
block_num_to_offset(bfsize_t block_num)
{
	bfsize_t colormaps = 1 + (block_num / BLOCK_COLOR_ENTRIES);

	return block_num + colormaps + 1;
}

/**
 * Convert a file offset in blocks to a block number.
 **/
static bfsize_t
offset_to_block_num(bfsize_t offset)
{
	offset--;
	offset -= offset / (BLOCK_COLOR_ENTRIES + 1);
	offset--;
	return offset;
}

/**
 * Convert a block number to an offset within a colormap. Optionally return the
 * colormap index.
 **/
static bfsize_t
block_num_to_colormap_offset(bfsize_t block_num, bfsize_t *colormap_idx)
{
	if (colormap_idx)
		*colormap_idx = block_num / BLOCK_COLOR_ENTRIES;
	return block_num % BLOCK_COLOR_ENTRIES;
}

/**
 * Turn a block offset to a colormap offset. Optionally return the colormap
 * index.
 **/
static bfsize_t
offset_to_colormap_offset(bfsize_t offset, bfsize_t *colormap_idx)
{
	return block_num_to_colormap_offset(offset_to_block_num(offset),
					    colormap_idx);
}

/**
 * Color a region in a color map.
 **/
static void
colormap_color(char *colormap, bfsize_t color_off, bfsize_t blocks, char color)
{
	bfsize_t start = color_off % 4;
	unsigned char mask;
	bfsize_t this_blocks;

	color = color | (color << 2) | (color << 4) | (color << 6);

	colormap += color_off / 4;

	while (blocks) {
		this_blocks = 4 - start;

		mask = 0xff >> (2 * start);

		if (blocks < this_blocks)
			this_blocks = blocks;

		mask ^= (mask >> (2 * this_blocks));

		*colormap &= ~mask;
		*colormap |= color & mask;

		blocks -= this_blocks;
		start = 0;
		colormap++;
	}
}

/**
 * Restore the colormap journal if needed.
 **/
static void
blockfile_restore_color_journal(blockfile_t *blockfile)
{
	unsigned char *journal = blockfile->metapage + BLOCK_MAGIC_LENGTH;
	unsigned char color;
	bfsize_t colormap;
	bfsize_t colormap_offset;
	bfsize_t count;

	if (crc64(journal, BLOCK_COLOR_JOURNAL_LENGTH))
		return;

	color = *journal;

	colormap_offset =
		block_num_to_colormap_offset(
				     be64toh(*(uint64_t *)(journal + 1)),
				     &colormap);

	count = be64toh(*(uint64_t *)(journal + 9));
	colormap_color(blockfile->colormaps[colormap], colormap_offset, count,
		       color);
	msync(blockfile->colormaps[colormap], BLOCK_SIZE, MS_SYNC);
}

/**
 * Open a block file.
 **/
blockfile_t *
blockfile_open(const char *path)
{
	blockfile_t *ret = xmalloc(sizeof(blockfile_t));
	int flags = O_CLOEXEC | O_RDWR;

	for (;;) {
		ret->fd = open(path, flags, 0600);

		if (ret->fd >= 0)
			break;

		if (errno != ENOENT)
			return NULL;

		if (flags & O_CREAT)
			return NULL;

		flags |= O_CREAT;
	}

	if (flags & O_CREAT) {
		if (ftruncate(ret->fd, BLOCK_SIZE * 2)) {
			close(ret->fd);
			return NULL;
		}
	}

	ret->mapped = NULL;
	ret->mapped_count = 0;
	ret->refcount = 1;

	ret->metapage = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE,
			      MAP_SHARED, ret->fd, 0);

	if (ret->metapage == MAP_FAILED) {
		close(ret->fd);
		free(ret);
		return NULL;
	}

	if (flags & O_CREAT) {
		memcpy(ret->metapage, BLOCK_MAGIC, BLOCK_MAGIC_LENGTH);
		memset(ret->metapage + BLOCK_MAGIC_LENGTH, 0,
		       BLOCK_COLOR_JOURNAL_LENGTH);
		((char *)ret->metapage)[BLOCK_MAGIC_LENGTH +
			BLOCK_COLOR_JOURNAL_LENGTH] = 0;
		msync(ret->metapage, BLOCK_SIZE, MS_SYNC);
	}

	if (blockfile_init_colormaps(ret)) {
		munmap(ret->metapage, BLOCK_SIZE);
		close(ret->fd);
		free(ret);
		return NULL;
	}

	if (flags & O_CREAT) {
		memset(ret->colormaps[0], 0, BLOCK_SIZE);
		msync(ret->colormaps[0], BLOCK_SIZE, MS_SYNC);
	} else if (! blockfile_validate(ret)) {
		blockfile_close(ret);
		return NULL;
	} else {
		blockfile_restore_color_journal(ret);
	}

	return ret;
}

/**
 * Get a reference to a blockfile.
 **/
blockfile_t *
blockfile_get_ref(blockfile_t *blockfile)
{
	blockfile->refcount++;
	return blockfile;
}

/**
 * Close a blockfile.
 **/
void
blockfile_close(blockfile_t *blockfile)
{
	bfsize_t i;

	if (--blockfile->refcount)
		return;

	for (i = 0; i < blockfile->mapped_count; i++)
		munmap(blockfile->mapped[i].mem_loc,
		       blockfile->mapped[i].length * BLOCK_SIZE);

	munmap(blockfile->metapage, BLOCK_SIZE);

	for (i = 0; i < blockfile->colormap_count; i++);
		munmap(blockfile->colormaps[i], BLOCK_SIZE);

	free(blockfile->colormaps);
	free(blockfile->mapped);
	close(blockfile->fd);
	free(blockfile);
}

/**
 * Turn a colormap offset to a block number.
 **/
static bfsize_t
colormap_offset_to_block_num(bfsize_t color_off, bfsize_t colormap_idx)
{
	return color_off + colormap_idx * BLOCK_COLOR_ENTRIES;
}

/**
 * Turn a colormap offset to a block offset.
 **/
static bfsize_t
colormap_offset_to_offset(bfsize_t color_off, bfsize_t colormap_idx)
{
	return block_num_to_offset(colormap_offset_to_block_num(color_off,
								colormap_idx));
}

/**
 * Find the color for a given colormap entry.
 **/
static inline char
probe_colormap(char *colormap, bfsize_t color_offset)
{
	char color = colormap[color_offset / 4];

	color_offset %= 4;
	color_offset = 3 - color_offset;
	color_offset *= 2;

	color >>= color_offset;
	color &= 3;

	return color;
}

/**
 * Get the dimensions of an allocated segment.
 **/
static bfsize_t
block_allocation_dimensions(blockfile_t *blockfile, bfsize_t *block_offset)
{
	bfsize_t colormap_idx;
	bfsize_t color_offset = offset_to_colormap_offset(*block_offset,
							&colormap_idx);
	bfsize_t end_offset = color_offset;
	char color;
	bfsize_t ret = 1;
	void *colormap;

	if (end_offset >= BLOCK_COLOR_ENTRIES)
		errx(1, "Multiple colormaps not yet supported.");

	if (colormap_idx >= blockfile->colormap_count)
		return 0;

	colormap = blockfile->colormaps[colormap_idx];

	color = probe_colormap(colormap, color_offset);
	if (! color)
		return 0;

	for(; color_offset; color_offset--, (*block_offset)--)
		if (probe_colormap(colormap, color_offset - 1) != color)
			break;

	end_offset++;
	while (end_offset < BLOCK_COLOR_ENTRIES &&
	       probe_colormap(colormap, end_offset) == color) {
		end_offset++;
		ret++;
	}

	return ret;
}

/**
 * Map a section in the block file to memory.
 **/
void *
blockfile_map(blockfile_t *blockfile, uint64_t block_num)
{
	uint64_t block_offset = block_num_to_offset(block_num);
	bfsize_t real_block_offset = block_offset;
	bfsize_t size = block_allocation_dimensions(blockfile, &real_block_offset);
	void *mapping;
	size_t i;

	for (i = 0; i < blockfile->mapped_count; i++) {
		if (blockfile->mapped[i].offset != block_offset)
			continue;

		blockfile->mapped[i].refcount++;

		return blockfile->mapped[i].mem_loc;
	}

	/* Unallocated region */
	if (! size)
		return NULL;

	/* Specified mid-region block */
	if (real_block_offset != block_offset)
		return NULL;

	mapping = mmap(NULL, BLOCK_SIZE * size, PROT_READ | PROT_WRITE,
		       MAP_SHARED, blockfile->fd, BLOCK_SIZE * block_offset);

	if (mapping == MAP_FAILED)
		return NULL;

	blockfile->mapped = xrealloc(blockfile->mapped,
				     (blockfile->mapped_count + 1) *
				     sizeof(struct block_range));

	blockfile->mapped[blockfile->mapped_count].offset = block_offset;
	blockfile->mapped[blockfile->mapped_count].length = size;
	blockfile->mapped[blockfile->mapped_count].mem_loc = mapping;
	blockfile->mapped[blockfile->mapped_count].refcount = 0;
	blockfile->mapped_count++;

	return mapping;
}

/**
 * Sync blocks in a blockfile to disk.
 **/
void
blockfile_sync(blockfile_t *blockfile, void *addr, bfsize_t block_offset,
	       bfsize_t blocks, int wait)
{
	bfsize_t i;
	int flags;

	if (wait)
		flags = MS_SYNC;
	else
		flags = MS_ASYNC;

	for (i = 0; i < blockfile->mapped_count; i++) {
		if (blockfile->mapped[i].mem_loc != addr)
		 continue;

		if (blockfile->mapped[i].length <= block_offset)
		 return;

		break;
	}

	if (i == blockfile->mapped_count)
		return;

	msync(addr + block_offset * BLOCK_SIZE, blocks * BLOCK_SIZE, flags);
}

/**
 * Unmap a block file.
 **/
void
blockfile_unmap(blockfile_t *blockfile, void *addr)
{
	bfsize_t i;

	for (i = 0; i < blockfile->mapped_count; i++)
	        if (blockfile->mapped[i].mem_loc == addr)
	       	 break;

	if (i == blockfile->mapped_count)
	        return;

	if (--blockfile->mapped[i].refcount)
		return;

	munmap(addr, blockfile->mapped[i].length * BLOCK_SIZE);

	memmove(&blockfile->mapped[i], &blockfile->mapped[i+1],
		(blockfile->mapped_count - i - 1) *
		sizeof(struct block_range));

	blockfile->mapped_count--;
}

/**
 * Ensure we have space for the block at the given offset.
 **/
static int
blockfile_ensure_space(blockfile_t *blockfile, bfsize_t last_block_offset)
{
	off_t end = lseek(blockfile->fd, 0, SEEK_END);

	if (end < 0)
		err(1, "Could not lseek blockfile");

	if ((bfsize_t)end / BLOCK_SIZE > last_block_offset)
		return 1;

	if (! ftruncate(blockfile->fd,
			last_block_offset * BLOCK_SIZE + BLOCK_SIZE))
		return 1;

	return 0;
}

/**
 * Find the largest free region in a colormap.
 **/
static void
colormap_get_max_region(void *colormap, bfsize_t *max_start, bfsize_t *max_count)
{
	bfsize_t count = 0;
	bfsize_t start = 0xdeadbeef; /* Makes compiler happy */
	bfsize_t i;

	*max_start = 0;
	*max_count = 0;

	for (i = 0; i < BLOCK_COLOR_ENTRIES; i++) {
		if (probe_colormap(colormap, i)) {
			count = 0;
			continue;
		}

		if (! count)
			start = i;

		count++;

		if (count > *max_count) {
			*max_count = count;
			*max_start = start;
		}
	}
}

/**
 * Allocate a new colormap.
 **/
static void *
colormap_allocate(blockfile_t *blockfile)
{
	bfsize_t pos = 1 + (BLOCK_COLOR_ENTRIES + 1) * blockfile->colormap_count;
	void *mapping;

	if (ftruncate(blockfile->fd, (pos + 1) * BLOCK_SIZE))
		return NULL;

	mapping = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
		       blockfile->fd, pos * BLOCK_SIZE);

	if (mapping == MAP_FAILED)
		return NULL;

	blockfile->colormaps = xrealloc(blockfile->colormaps,
					(blockfile->colormap_count + 1) *
					sizeof(void *));

	blockfile->colormaps[blockfile->colormap_count++] = mapping;

	return mapping;
}

/**
 * Write to a colormap journal.
 **/
static void
colormap_journal_commit(void *metapage, bfsize_t start, bfsize_t blocks,
			char color)
{
	char *pos = metapage + BLOCK_MAGIC_LENGTH;

	*(char *)(pos) = color;
	*(uint64_t *)(pos + 1) = htobe64(start);
	*(uint64_t *)(pos + 9) = htobe64(blocks);
	*(uint64_t *)(pos + 17) = htobe64(crc64(metapage, 17));
	msync(metapage, BLOCK_SIZE, MS_SYNC);
}

/**
 * Allocate a region in a block file.
 **/
block_t
blockfile_allocate(blockfile_t *blockfile, bfsize_t blocks)
{
	bfsize_t i;
	bfsize_t max_count = 0;
	bfsize_t max_start = 0;
	bfsize_t max_end;
	bfsize_t last_block_off;
	bfsize_t ret;
	void *colormap = NULL;

	char color_before = 0;
	char color_after = 0;
	char color;

	if (! blocks)
		return BLOCK_BAD_ARGUMENT;

	if (blocks > BLOCK_COLOR_ENTRIES)
		return BLOCK_BAD_ARGUMENT;

	for (i = 0; i < blockfile->colormap_count; i++) {
		colormap = blockfile->colormaps[i];

		colormap_get_max_region(colormap, &max_start, &max_count);

		if (max_count >= blocks)
			break;
	}

	if (max_count < blocks) {
		colormap = colormap_allocate(blockfile);

		if (! colormap)
			return BLOCK_BAD_NOSPACE;

		max_start = 0;
		max_count = BLOCK_COLOR_ENTRIES;
		i = blockfile->colormap_count - 1;
	}

	max_end = max_start + max_count;

	if (max_start)
		color_before = probe_colormap(colormap, max_start - 1);

	if (max_end < BLOCK_COLOR_ENTRIES)
		color_after = probe_colormap(colormap, max_end);

	color = 1;

	if (color_before == color || color_after == color)
		color = 2;
	if (color_before == color || color_after == color)
		color = 3;

	ret = colormap_offset_to_block_num(max_start, i);
	last_block_off = colormap_offset_to_offset(max_start, i) + blocks - 1;

	if (! blockfile_ensure_space(blockfile, last_block_off))
		return BLOCK_BAD_NOSPACE;

	colormap_journal_commit(blockfile->metapage, ret, blocks, color);
	colormap_color(colormap, max_start, blocks, color);
	msync(colormap, BLOCK_SIZE, MS_SYNC);

	return ret;
}

/**
 * Free a region in a blockfile.
 **/
block_t
blockfile_free(blockfile_t *blockfile, block_t block_num)
{
	bfsize_t colormap_idx;
	bfsize_t color_off = block_num_to_colormap_offset(block_num,
							&colormap_idx);
	bfsize_t block_offset = block_num_to_offset(block_num);
	bfsize_t real_block_offset = block_offset;
	bfsize_t size = block_allocation_dimensions(blockfile, &real_block_offset);
	bfsize_t i;
	void *colormap;

	if (size == 0)
		return BLOCK_BAD_ARGUMENT;

	if (real_block_offset != block_offset)
		return BLOCK_BAD_ARGUMENT;

	if (colormap_idx > blockfile->colormap_count)
		return BLOCK_BAD_ARGUMENT;

	colormap = blockfile->colormaps[colormap_idx];

	if (! probe_colormap(colormap, color_off))
		return BLOCK_BAD_ARGUMENT;

	for (i = 0; i < blockfile->mapped_count; i++)
		if (blockfile->mapped[i].offset == block_offset)
			return block_num;

	colormap_journal_commit(blockfile->metapage, block_num, size, 0);
	colormap_color(colormap, color_off, size, 0);
	msync(colormap, BLOCK_SIZE, MS_SYNC);
	return BLOCK_BAD_MISSING;
}

/**
 * Annotate a given block. Annotations are string tags which can be applied to
 * any one block. If the annotation already exists, it is altered to point to
 * the given block. The annotation must be less than 256 bytes long.
 **/
block_t
blockfile_annotate_block(blockfile_t *blockfile, block_t block_num, const char *name)
{
	unsigned char *metapage = blockfile->metapage;
	bfsize_t pos = BLOCK_MAGIC_LENGTH + BLOCK_COLOR_JOURNAL_LENGTH;
	bfsize_t namelen = strlen(name);
	bfsize_t space = namelen + 10;

	if (! namelen)
		return BLOCK_BAD_ARGUMENT;

	if (namelen > 0xff)
		return BLOCK_BAD_NOSPACE;

	while (pos < BLOCK_SIZE && metapage[pos]) {
		if (metapage[pos] != namelen) {
			pos += metapage[pos] + 9;
			continue;
		}

		pos++;

		if (strncmp((char *)&metapage[pos], name, namelen)) {
			pos += namelen + 8;
			continue;
		}

		pos += namelen;
		goto write_value;
	}

	if (pos + space > BLOCK_SIZE)
		return BLOCK_BAD_NOSPACE;

	metapage[pos++] = (unsigned char)namelen;
	memcpy(&metapage[pos], name, namelen);
	pos += namelen;

	metapage[pos + 8] = 0;

write_value:
	*(uint64_t *)&metapage[pos] = htobe64(block_num);
	pos += 8;

	return block_num;
}

/**
 * Remove a given block annotation.
 **/
void
blockfile_remove_annotation(blockfile_t *blockfile, const char *name)
{
	unsigned char *metapage = blockfile->metapage;
	bfsize_t pos = BLOCK_MAGIC_LENGTH + BLOCK_COLOR_JOURNAL_LENGTH;
	bfsize_t end_pos;
	bfsize_t next_pos;
	bfsize_t namelen = strlen(name);

	if (! namelen)
		return;

	if (namelen > 0xff)
		return;

	while (pos < BLOCK_SIZE && metapage[pos]) {
		if (metapage[pos] != namelen) {
			pos += metapage[pos] + 9;
			continue;
		}

		if (! strncmp((char *)&metapage[pos + 1], name, namelen))
			break;

		pos += namelen + 9;
		continue;
	}

	if (pos >= BLOCK_SIZE)
		errx(1, "Corrupt annotation list");

	if (! metapage[pos])
		return;

	end_pos = pos;
	end_pos += namelen + 9;
	next_pos = end_pos;

	while (end_pos < BLOCK_SIZE && metapage[end_pos])
		end_pos += metapage[end_pos] + 9;

	if (end_pos >= BLOCK_SIZE)
		errx(1, "Corrupt annotation list");

	memmove(&metapage[pos], &metapage[next_pos], end_pos - next_pos + 1);
	msync(metapage, BLOCK_SIZE, MS_SYNC);
}

/**
 * Get the block with a given annotation.
 **/
block_t
blockfile_get_annotated_block(blockfile_t *blockfile, const char *name)
{
	unsigned char *metapage = blockfile->metapage;
	bfsize_t pos = BLOCK_MAGIC_LENGTH + BLOCK_COLOR_JOURNAL_LENGTH;
	bfsize_t namelen = strlen(name);

	if (! namelen)
		return BLOCK_BAD_ARGUMENT;

	if (namelen > 0xff)
		return BLOCK_BAD_ARGUMENT;

	while (pos < BLOCK_SIZE && metapage[pos]) {
		if (metapage[pos] != namelen) {
			pos += metapage[pos] + 9;
			continue;
		}

		pos++;

		if (! strncmp((char *)&metapage[pos], name, namelen))
			break;

		pos += namelen + 8;
		continue;
	}

	if (pos >= BLOCK_SIZE)
		errx(1, "Corrupt annotation list");

	if (! metapage[pos])
		return BLOCK_BAD_MISSING;

	pos += namelen;

	return be64toh(*(uint64_t *)&metapage[pos]);
}
