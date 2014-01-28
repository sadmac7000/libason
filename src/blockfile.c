/**
 * Copyright © 2013, 2014 Red Hat, Casey Dahlin <cdahlin@redhat.com>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "blockfile.h"
#include "util.h"

/**
 * A range of blocks.
 **/
struct block_range {
	size_t offset;
	size_t length;
	void *mem_loc;
};

/**
 * A block file.
 **/
struct blockfile {
	int fd;
	struct block_range *mapped;
	size_t mapped_count;
	void *metapage;
	void *colormap;
};

/**
 * Confirm the magic on a blockfile.
 **/
static int
blockfile_validate(blockfile_t *blockfile)
{
	return !!strncmp(blockfile->metapage, BLOCK_MAGIC, BLOCK_MAGIC_LENGTH);
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

	ret->metapage = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE,
			      MAP_SHARED, ret->fd, 0);

	if (! ret->metapage) {
		close(ret->fd);
		free(ret);
		return NULL;
	}

	if (flags & O_CREAT) {
		memcpy(ret->metapage, BLOCK_MAGIC, BLOCK_MAGIC_LENGTH);
		((char *)ret->metapage)[BLOCK_MAGIC_LENGTH] = 0;
		msync(ret->metapage, BLOCK_SIZE, MS_SYNC);
	}

	ret->colormap = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE,
			     MAP_SHARED, ret->fd, BLOCK_SIZE);

	if (! ret->colormap) {
		munmap(ret->metapage, BLOCK_SIZE);
		close(ret->fd);
		free(ret);
		return NULL;
	}

	if (flags & O_CREAT) {
		memset(ret->colormap, 0, BLOCK_SIZE);
		msync(ret->colormap, BLOCK_SIZE, MS_SYNC);
	} else if (! blockfile_validate(ret)) {
		blockfile_close(ret);
		return NULL;
	}

	return ret;
}

/**
 * Close a blockfile.
 **/
void
blockfile_close(blockfile_t *blockfile)
{
	size_t i;

	for (i = 0; i < blockfile->mapped_count; i++)
		munmap(blockfile->mapped[i].mem_loc,
		       blockfile->mapped[i].length * BLOCK_SIZE);

	munmap(blockfile->metapage, BLOCK_SIZE);
	munmap(blockfile->colormap, BLOCK_SIZE);
	free(blockfile->mapped);
	close(blockfile->fd);
	free(blockfile);
}

/**
 * Convert block numbers to file offsets in blocks. Block numbers don't count
 * the metadata or the colormap blocks.
 **/
static size_t
block_num_to_offset(size_t block_num)
{
	size_t colormaps = 1 + (block_num / BLOCK_COLOR_ENTRIES);

	return block_num + colormaps + 1;
}

/**
 * Convert a file offset in blocks to a block number.
 **/
static size_t
offset_to_block_num(size_t offset)
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
static size_t
block_num_to_colormap_offset(size_t block_num, size_t *colormap_idx)
{
	if (colormap_idx)
		*colormap_idx = block_num / BLOCK_COLOR_ENTRIES;
	return block_num % BLOCK_COLOR_ENTRIES;
}

/**
 * Turn a block offset to a colormap offset. Optionally return the colormap
 * index.
 **/
static size_t
offset_to_colormap_offset(size_t offset, size_t *colormap_idx)
{
	return block_num_to_colormap_offset(offset_to_block_num(offset),
					    colormap_idx);
}

/**
 * Turn a colormap offset to a block number.
 **/
static size_t
colormap_offset_to_block_num(size_t color_off, size_t colormap_idx)
{
	return color_off + colormap_idx * BLOCK_COLOR_ENTRIES;
}

/**
 * Turn a colormap offset to a block offset.
 **/
static size_t
colormap_offset_to_offset(size_t color_off, size_t colormap_idx)
{
	return block_num_to_offset(colormap_offset_to_block_num(color_off,
								colormap_idx));
}

/**
 * Find the color for a given colormap entry.
 **/
static inline char
probe_colormap(char *colormap, size_t color_offset)
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
static size_t
block_allocation_dimensions(blockfile_t *blockfile, size_t *block_offset)
{
	size_t color_offset = offset_to_colormap_offset(*block_offset, NULL);
	size_t end_offset = color_offset;
	char color;
	size_t ret = 1;

	if (end_offset >= BLOCK_COLOR_ENTRIES)
		errx(1, "Multiple colormaps not yet supported.");

	color = probe_colormap(blockfile->colormap, color_offset);
	if (! color)
		return 0;

	for(; color_offset; color_offset--, (*block_offset)--)
		if (probe_colormap(blockfile->colormap,
				   color_offset - 1) != color)
			break;

	end_offset++;
	while (end_offset < BLOCK_COLOR_ENTRIES &&
	       probe_colormap(blockfile->colormap, end_offset) == color) {
		end_offset++;
		ret++;
	}

	return ret;
}

/**
 * Map a section in the block file to memory.
 **/
void *
blockfile_map(blockfile_t *blockfile, size_t block_num)
{
	size_t block_offset = block_num_to_offset(block_num);
	size_t real_block_offset = block_offset;
	size_t size = block_allocation_dimensions(blockfile, &real_block_offset);
	void *mapping;

	/* Unallocated region */
	if (! size)
		return NULL;

	/* Specified mid-region block */
	if (real_block_offset != block_offset)
		return NULL;

	mapping = mmap(NULL, BLOCK_SIZE * size, PROT_READ | PROT_WRITE,
		       MAP_SHARED, blockfile->fd, BLOCK_SIZE * block_offset);

	if (! mapping)
		return NULL;

	blockfile->mapped = xrealloc(blockfile->mapped,
				     (blockfile->mapped_count + 1) *
				     sizeof(struct block_range));

	blockfile->mapped[blockfile->mapped_count].offset = block_offset;
	blockfile->mapped[blockfile->mapped_count].length = size;
	blockfile->mapped[blockfile->mapped_count].mem_loc = mapping;
	blockfile->mapped_count++;

	return mapping;
}

/**
 * Sync blocks in a blockfile to disk.
 **/
void
blockfile_sync(blockfile_t *blockfile, void *addr, size_t block_offset,
	       size_t blocks, int wait)
{
	size_t i;
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
	size_t i;

	for (i = 0; i < blockfile->mapped_count; i++)
	        if (blockfile->mapped[i].mem_loc == addr)
	       	 break;

	if (i == blockfile->mapped_count)
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
blockfile_ensure_space(blockfile_t *blockfile, size_t last_block_offset)
{
	off_t end = lseek(blockfile->fd, 0, SEEK_END);

	if (end < 0)
		err(1, "Could not lseek blockfile");

	if ((size_t)end / BLOCK_SIZE > last_block_offset)
		return 1;

	if (! ftruncate(blockfile->fd,
			last_block_offset * BLOCK_SIZE + BLOCK_SIZE))
		return 1;

	return 0;
}

/**
 * Color a region in a color map.
 **/
static void
colormap_color(char *colormap, size_t color_off, size_t blocks, char color)
{
	size_t start = color_off % 4;
	unsigned char mask;
	size_t this_blocks;

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
 * Allocate a region in a block file.
 **/
ssize_t
blockfile_allocate(blockfile_t *blockfile, size_t blocks)
{
	size_t i;
	size_t count = 0;
	size_t start;
	size_t max_count = 0;
	size_t max_start = 0;
	size_t max_end = 0;
	size_t last_block_off;
	size_t ret;

	char color_before = 0;
	char color_after = 0;
	char color;

	if (! blocks)
		return -EINVAL;

	if (blocks > BLOCK_COLOR_ENTRIES)
		return -EINVAL;

	for (i = 0; i < BLOCK_COLOR_ENTRIES; i++) {
		if (probe_colormap(blockfile->colormap, i)) {
			count = 0;
			continue;
		}

		if (! count)
			start = i;

		count++;

		if (count > max_count) {
			max_count = count;
			max_start = start;
		}
	}

	if (max_count < blocks)
		return -ENOSPC;

	max_end = max_start + max_count;

	if (max_start)
		color_before = probe_colormap(blockfile->colormap,
					      max_start - 1);

	if (max_end < BLOCK_COLOR_ENTRIES)
		color_after = probe_colormap(blockfile->colormap, max_end);

	color = 1;

	if (color_before == color || color_after == color)
		color = 2;
	if (color_before == color || color_after == color)
		color = 3;

	ret = colormap_offset_to_block_num(max_start, 0);
	last_block_off = colormap_offset_to_offset(max_start, 0) + blocks - 1;

	if (! blockfile_ensure_space(blockfile, last_block_off))
		return -ENOSPC;

	colormap_color(blockfile->colormap, max_start, blocks, color);
	msync(blockfile->colormap, BLOCK_SIZE, MS_SYNC);

	return ret;
}

/**
 * Free a region in a blockfile.
 **/
int
blockfile_free(blockfile_t *blockfile, size_t block_num)
{
	size_t color_off = block_num_to_colormap_offset(block_num, NULL);
	size_t block_offset = block_num_to_offset(block_num);
	size_t real_block_offset = block_offset;
	size_t size = block_allocation_dimensions(blockfile, &real_block_offset);
	size_t i;

	if (size == 0)
		return -EINVAL;

	if (real_block_offset != block_offset)
		return -EINVAL;

	for (i = 0; i < blockfile->mapped_count; i++)
		if (blockfile->mapped[i].offset == block_offset)
			return -EDEADLK;

	colormap_color(blockfile->colormap, color_off, size, 0);
	msync(blockfile->colormap, BLOCK_SIZE, MS_SYNC);
	return 0;
}

/**
 * Annotate a given block. Annotations are string tags which can be applied to
 * any one block. If the annotation already exists, it is altered to point to
 * the given block.
 **/
int
blockfile_annotate_block(blockfile_t *blockfile, size_t block_num, const char *name)
{
	(void)blockfile;
	(void)block_num;
	(void)name;
	return -ENOTSUP;
}

/**
 * Remove a given block annotation.
 **/
int
blockfile_remove_annotation(blockfile_t *blockfile, const char *name)
{
	(void)blockfile;
	(void)name;
	return -ENOTSUP;
}

/**
 * Get the block with a given annotation.
 **/
ssize_t
blockfile_get_annotated_block(blockfile_t *blockfile, const char *name)
{
	(void)blockfile;
	(void)name;
	return -ENOENT;
}
