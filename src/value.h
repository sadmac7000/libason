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
#include <stdint.h>

#include <ason/value.h>

/**
 * An ASON type.
 **/
typedef enum {
	TYPE_NUMERIC,
	TYPE_EMPTY,
	TYPE_NULL,
	TYPE_UNIVERSE,
	TYPE_WILD,
	TYPE_UNION,
	TYPE_INTERSECT,
	TYPE_OBJECT,
	TYPE_UOBJECT,
	TYPE_LIST,
	TYPE_APPEND,
	TYPE_COMP,
	TYPE_STRING,
	TYPE_TRUE,
	TYPE_FALSE,
	TYPE_REPR,
	TYPE_EQUAL,
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
		char *string;
	};

	size_t count;
	size_t refcount;
};

/**
 * Test if an ASON value is an object.
 **/
#define IS_OBJECT(_x) (_x->type == TYPE_OBJECT || _x->type == TYPE_UOBJECT)

/**
 * Test if an ASON value is a boolean.
 **/
#define IS_BOOL(_x) (_x->type == TYPE_TRUE || _x->type == TYPE_FALSE)

#ifdef __cplusplus
extern "C" {
#endif

ason_t * ason_create_fixnum(int64_t number);
int ason_reduce(ason_t *value);

#ifdef __cplusplus
}
#endif

#endif /* VALUE_H */
