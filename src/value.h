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

#ifndef VALUE_H
#define VALUE_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include <ason/ason.h>

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
	int order;
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
#define IS_OBJECT(_x) (_x->type == ASON_TYPE_OBJECT || \
		       _x->type == ASON_TYPE_UOBJECT)

/**
 * Test if an ASON value is a boolean.
 **/
#define IS_BOOL(_x) (_x->type == ASON_TYPE_TRUE || \
		     _x->type == ASON_TYPE_FALSE)

#ifdef __cplusplus
extern "C" {
#endif

ason_t *ason_create_number(int64_t number);
ason_t *ason_create_list(ason_t *content);
ason_t *ason_append_lists(ason_t *list, ason_t *item);
ason_t *ason_create_object(const char *key, ason_t *value); 
ason_t *ason_create_string(const char *str);
ason_t *ason_union(ason_t *a, ason_t *b);
ason_t *ason_intersect(ason_t *a, ason_t *b);
ason_t *ason_join(ason_t *a, ason_t *b);
ason_t *ason_complement(ason_t *a);
ason_t *ason_representation_in(ason_t *a, ason_t *b);
ason_t *ason_equality(ason_t *a, ason_t *b);

ason_t * ason_create_fixnum(int64_t number);
int ason_reduce(ason_t *value);

/* Destructive operators */

static inline ason_t *
ason_create_list_d(ason_t *content)
{
	ason_t *ret = ason_create_list(content);
	ason_destroy(content);
	return ret;
}

static inline ason_t *
ason_create_object_d(const char *key, ason_t *value)
{
	ason_t *ret = ason_create_object(key, value);
	ason_destroy(value);
	return ret;
}

static inline ason_t *
ason_union_d(ason_t *a, ason_t *b)
{
	ason_t *ret;
	ret = ason_union(a, b);
	ason_destroy(a);
	ason_destroy(b);
	return ret;
}

static inline ason_t *
ason_intersect_d(ason_t *a, ason_t *b)
{
	ason_t *ret;
	ret = ason_intersect(a, b);
	ason_destroy(a);
	ason_destroy(b);
	return ret;
}

static inline ason_t *
ason_join_d(ason_t *a, ason_t *b)
{
	ason_t *ret;
	ret = ason_join(a, b);
	ason_destroy(a);
	ason_destroy(b);
	return ret;
}

static inline ason_t *
ason_complement_d(ason_t *a)
{
	ason_t *ret;
	ret = ason_complement(a);
	ason_destroy(a);
	return ret;
}

static inline ason_t *
ason_representation_in_d(ason_t *a, ason_t *b)
{
	ason_t *ret;
	ret = ason_representation_in(a, b);
	ason_destroy(a);
	ason_destroy(b);
	return ret;
}

static inline ason_t *
ason_equality_d(ason_t *a, ason_t *b)
{
	ason_t *ret;
	ret = ason_equality(a, b);
	ason_destroy(a);
	ason_destroy(b);
	return ret;
}

static inline ason_t *
ason_append_lists_d(ason_t *a, ason_t *b)
{
	ason_t *ret;
	ret = ason_append_lists(a, b);
	ason_destroy(a);
	ason_destroy(b);
	return ret;
}

#ifdef __cplusplus
}
#endif

#endif /* VALUE_H */
