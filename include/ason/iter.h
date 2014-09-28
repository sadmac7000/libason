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

#ifndef ASON_ITER_H
#define ASON_ITER_H

#include <ason/value.h>

typedef struct ason_iter ason_iter_t;

#ifdef __cplusplus
extern "C" {
#endif

ason_iter_t *ason_iterate(ason_t *value);
int ason_iter_enter(ason_iter_t *iter);
int ason_iter_exit(ason_iter_t *iter);
int ason_iter_next(ason_iter_t *iter);
int ason_iter_prev(ason_iter_t *iter);
ason_type_t ason_iter_type(ason_iter_t *iter);
long long ason_iter_long(ason_iter_t *iter);
double ason_iter_double(ason_iter_t *iter);
char *ason_iter_string(ason_iter_t *iter);
char *ason_iter_key(ason_iter_t *iter);
ason_t *ason_iter_value(ason_iter_t *iter);
void ason_iter_destroy(ason_iter_t *iter);

#ifdef __cplusplus
}
#endif

#endif /* ASON_ITER_H */
