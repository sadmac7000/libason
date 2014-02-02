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

#include <stdint.h>

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

/**
 * Invalid block values for returning.
 **/
#define BLOCK_BAD_ARGUMENT    0xffffffffffffffff
#define BLOCK_BAD_NOSPACE     0xfffffffffffffffe
#define BLOCK_BAD_MISSING     0xfffffffffffffffd

#define BLOCK_ERROR_START BLOCK_BAD_MISSING

typedef struct blockfile blockfile_t;
typedef uint64_t block_t;
typedef uint64_t bfsize_t;

#ifdef __cplusplus
extern "C" {
#endif

blockfile_t *blockfile_open(const char *path);
void blockfile_close(blockfile_t *blockfile);
void *blockfile_map(blockfile_t *blockfile, block_t block_num);
void blockfile_sync(blockfile_t *blockfile, void *addr, bfsize_t block_offset,
		    bfsize_t blocks, int wait);
void blockfile_unmap(blockfile_t *blockfile, void *addr);
block_t blockfile_allocate(blockfile_t *blockfile, bfsize_t blocks);
block_t blockfile_free(blockfile_t *blockfile, block_t block_num);
block_t blockfile_annotate_block(blockfile_t *blockfile, block_t block_num,
				 const char *name);
void blockfile_remove_annotation(blockfile_t *blockfile, const char *name);
block_t blockfile_get_annotated_block(blockfile_t *blockfile,
				      const char *name);
blockfile_t *blockfile_get_ref(blockfile_t *blockfile);

#ifdef __cplusplus
}
#endif

#endif /* BLOCKFILE_H */
