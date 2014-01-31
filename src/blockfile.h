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

#ifndef BLOCKFILE_H
#define BLOCKFILE_H

/**
 * Size of a single block in a block file. This will be the standard page size
 * on most popular arches. We can optimize around an 8k page with this block
 * size without too much effort. The size itself shouldn't have to change until
 * pages get much, much larger.
 **/
#define BLOCK_SIZE 4096

/**
 * How many color entries in a one-block colormap.
 **/
#define BLOCK_COLOR_ENTRIES (BLOCK_SIZE * 8 / 2)

/**
 * Blockfile magic.
 **/
#define BLOCK_MAGIC "asonblok\0\0\0\0\0\0\0"
#define BLOCK_MAGIC_LENGTH 15
#define BLOCK_COLOR_JOURNAL_LENGTH 25

typedef struct blockfile blockfile_t;

#ifdef __cplusplus
extern "C" {
#endif

blockfile_t *blockfile_open(const char *path);
void blockfile_close(blockfile_t *blockfile);
void *blockfile_map(blockfile_t *blockfile, size_t block_num);
void blockfile_sync(blockfile_t *blockfile, void *addr, size_t block_offset,
		    size_t blocks, int wait);
void blockfile_unmap(blockfile_t *blockfile, void *addr);
ssize_t blockfile_allocate(blockfile_t *blockfile, size_t blocks);
int blockfile_free(blockfile_t *blockfile, size_t block_num);
int blockfile_annotate_block(blockfile_t *blockfile, size_t block_num,
			     const char *name);
void blockfile_remove_annotation(blockfile_t *blockfile, const char *name);
ssize_t blockfile_get_annotated_block(blockfile_t *blockfile,
				      const char *name);

#ifdef __cplusplus
}
#endif

#endif /* BLOCKFILE_H */
