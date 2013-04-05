/**
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

#ifndef VALUE_H
#define VALUE_H

#include <stdio.h>
#include <stdarg.h>

#include <ason/value.h>

/**
 * An ASON type.
 **/
typedef enum {
	ASON_NUMERIC,
	ASON_EMPTY,
	ASON_NULL,
	ASON_UNIVERSE,
	ASON_WILD,
	ASON_UNION,
	ASON_INTERSECT,
	ASON_OBJECT,
	ASON_UOBJECT,
	ASON_LIST,
	ASON_APPEND,
	ASON_COMP,
} ason_type_t;

/**
 * A Key-value pair.
 **/
struct kv_pair {
	char *key;
	ason_t *value;
};

/**
 * Data making up a value.
 **/
struct ason {
	ason_type_t type;
	union {
		int64_t n;
		ason_t **items;
		struct kv_pair *kvs;
	};

	size_t count;
	size_t refcount;
};

/**
 * Test if an ASON value is an object.
 **/
#define IS_OBJECT(_x) (_x->type == ASON_OBJECT || _x->type == ASON_UOBJECT)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* VALUE_H */
