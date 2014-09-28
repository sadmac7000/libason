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

#ifndef ASON_VALUE_H
#define ASON_VALUE_H

#include <stdint.h>

/**
 * An ASON type.
 **/
typedef enum {
	ASON_TYPE_NUMERIC,
	ASON_TYPE_EMPTY,
	ASON_TYPE_NULL,
	ASON_TYPE_UNION,
	ASON_TYPE_INTERSECT,
	ASON_TYPE_OBJECT,
	ASON_TYPE_UOBJECT,
	ASON_TYPE_LIST,
	ASON_TYPE_JOIN,
	ASON_TYPE_COMP,
	ASON_TYPE_STRING,
	ASON_TYPE_TRUE,
	ASON_TYPE_FALSE,
	ASON_TYPE_REPR,
	ASON_TYPE_EQUAL,
} ason_type_t;

/**
 * An ASON value
 **/
typedef struct ason ason_t;

extern ason_t * const ASON_EMPTY;
extern ason_t * const ASON_NULL;
extern ason_t * const ASON_UNIVERSE;
extern ason_t * const ASON_WILD;
extern ason_t * const ASON_OBJ_ANY;
extern ason_t * const ASON_TRUE;
extern ason_t * const ASON_FALSE;

#ifdef __cplusplus
extern "C" {
#endif

ason_t *ason_copy(ason_t *a);
void ason_destroy(ason_t *a);

int ason_check_represented_in(ason_t *a, ason_t *b);
int ason_check_equal(ason_t *a, ason_t *b);

ason_type_t ason_type(ason_t *a);
long long ason_long(ason_t *a);
double ason_double(ason_t *a);
char *ason_string(ason_t *a);

#ifdef __cplusplus
}
#endif

#endif /* ASON_VALUE_H */
