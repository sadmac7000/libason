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

#ifndef BTREE_H
#define BTREE_H

#include <stdint.h>

#include "blockfile.h"

typedef struct btree btree_t;

/**
 * Creation flags for B trees.
 **/
/* None now */

#ifdef __cplusplus
extern "C" {
#endif

btree_t *btree_create(blockfile_t *blockfile, uint32_t flags);
btree_t *btree_load_headblock(blockfile_t *blockfile, size_t block_num);
btree_t *btree_load_annotation(blockfile_t *blockfile, const char *annotation);
int btree_insert(btree_t *btree, uint64_t key, uint64_t data);
int btree_delete(btree_t *btree, uint64_t key);
uint64_t btree_lookup(btree_t *btree, uint64_t key);

#ifdef __cplusplus
}
#endif

#endif /* BTREE_H */

