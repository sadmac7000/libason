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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <err.h>

#include <ason/value.h>

#include "value.h"
#include "util.h"
#include "stringfunc.h"

/* Special values for the order field. */
#define ORDER_UNKNOWN 5
#define ORDER_OF_EMPTY -1

/**
 * An iterator to iterate the keys in two objects at once.
 **/
struct ason_coiterator {
	size_t a_i;
	size_t b_i;
	ason_t *a;
	ason_t *b;
};

/**
 * Handy value constants.
 **/
static struct ason ASON_EMPTY_DATA = {
	.type = ASON_TYPE_EMPTY,
	.items = NULL,
	.count = 0,
	.order = ORDER_OF_EMPTY,
};
API_EXPORT ason_t * const ASON_EMPTY = &ASON_EMPTY_DATA;

static struct ason ASON_NULL_DATA = {
	.type = ASON_TYPE_NULL,
	.items = NULL,
	.count = 0,
	.order = 0,
};
API_EXPORT ason_t * const ASON_NULL = &ASON_NULL_DATA;

static struct ason ASON_UNIVERSE_DATA = {
	.type = ASON_TYPE_UNIVERSE,
	.items = NULL,
	.count = 0,
	.order = 2,
};
API_EXPORT ason_t * const ASON_UNIVERSE = &ASON_UNIVERSE_DATA;

static struct ason ASON_TRUE_DATA = {
	.type = ASON_TYPE_TRUE,
	.items = NULL,
	.count = 0,
	.order = 0,
};
API_EXPORT ason_t * const ASON_TRUE = &ASON_TRUE_DATA;

static struct ason ASON_FALSE_DATA = {
	.type = ASON_TYPE_FALSE,
	.items = NULL,
	.count = 0,
	.order = 0,
};
API_EXPORT ason_t * const ASON_FALSE = &ASON_FALSE_DATA;

static struct ason ASON_WILD_DATA = {
	.type = ASON_TYPE_WILD,
	.items = NULL,
	.count = 0,
	.order = 2,
};
API_EXPORT ason_t * const ASON_WILD = &ASON_WILD_DATA;

static struct ason ASON_OBJ_ANY_DATA = {
	.type = ASON_TYPE_UOBJECT,
	.items = NULL,
	.count = 0,
	.order = 3,
};
API_EXPORT ason_t * const ASON_OBJ_ANY = &ASON_OBJ_ANY_DATA;

/**
 * Quicksort an array of KV pairs.
 **/
static void
kv_pair_quicksort(struct kv_pair *kvs, size_t count)
{
	size_t pivot;
	size_t i;
	struct kv_pair tmp;

	if (count <= 1)
		return;

	pivot = 0;

	for (i = 1; i < count; i++) {
		if (strcmp(kvs[i].key, kvs[pivot].key) >= 0)
			continue;

		tmp = kvs[pivot + 1];
		kvs[pivot + 1] = kvs[pivot];
		kvs[pivot++] = kvs[i];
		kvs[i] = tmp;
	}

	kv_pair_quicksort(kvs, pivot);
	kv_pair_quicksort(kvs + pivot + 1, count - pivot - 1);
}

/**
 * Sort the KV pairs in an object.
 **/
static void
ason_object_sort_kvs(ason_t *obj)
{
	struct kv_pair *kvs = obj->kvs;
	size_t count = obj->count;

	kv_pair_quicksort(kvs, count);
}

/**
 * Initialize an ASON coiterator.
 **/
static void
ason_coiterator_init(struct ason_coiterator *iter, ason_t *a, ason_t *b)
{
	iter->a_i = 0;
	iter->b_i = 0;
	iter->a = ason_copy(a);
	iter->b = ason_copy(b);

	ason_object_sort_kvs(a);
	ason_object_sort_kvs(b);
}

/**
 * Get the next values for an ASON coiterator.
 **/
static const char *
ason_coiterator_next(struct ason_coiterator *iter, ason_t **a, ason_t **b)
{
	const char *ret;
	int cmp;
	*a = *b = ASON_NULL;

	if (iter->a->type == ASON_TYPE_UOBJECT)
		*a = ASON_UNIVERSE;

	if (iter->b->type == ASON_TYPE_UOBJECT)
		*b = ASON_UNIVERSE;

	if (iter->a_i >= iter->a->count && iter->b_i >= iter->b->count)
		return NULL;

	if (iter->a_i >= iter->a->count) {
		*b = iter->b->kvs[iter->b_i].value;
		return iter->b->kvs[iter->b_i++].key;
	}

	if (iter->b_i >= iter->b->count) {
		*a = iter->a->kvs[iter->a_i].value;
		return iter->a->kvs[iter->a_i++].key;
	}

	cmp = strcmp(iter->a->kvs[iter->a_i].key,
		     iter->b->kvs[iter->b_i].key);

	if (cmp <= 0) {
		ret = iter->a->kvs[iter->a_i].key;
		*a = iter->a->kvs[iter->a_i].value;
		iter->a_i++;
	}

	if (cmp >= 0) {
		ret = iter->b->kvs[iter->b_i].key;
		*b = iter->b->kvs[iter->b_i].value;
		iter->b_i++;
	}

	return ret;
}

/**
 * Release the resources of a coiterator.
 **/
static void
ason_coiterator_release(struct ason_coiterator *iter)
{
	ason_destroy(iter->a);
	ason_destroy(iter->b);
}

/**
 * Create a new ASON value struct.
 **/
static ason_t *
ason_create(ason_type_t type, size_t count)
{
	ason_t *ret = xcalloc(1, sizeof(struct ason));

	ret->type = type;
	ret->count = count;
	ret->refcount = 1;
	ret->order = ORDER_UNKNOWN;

	if (! ret->count)
		return ret;

	if (type == ASON_TYPE_OBJECT || type == ASON_TYPE_UOBJECT)
		ret->kvs = xcalloc(count, sizeof(struct kv_pair));
	else
		ret->items = xcalloc(count, sizeof(ason_t *));

	return ret;
}

/**
 * Create an ASON string value.
 **/
API_EXPORT ason_t *
ason_create_string(const char *string)
{
	ason_t *ret = ason_create(ASON_TYPE_STRING, 0);
	ret->string = string_to_utf8(string);
	return ret;
}

/**
 * Copy an ASON value.
 **/
API_EXPORT ason_t *
ason_copy(ason_t *a)
{
	if (a == ASON_EMPTY	||
	    a == ASON_NULL	||
	    a == ASON_UNIVERSE	||
	    a == ASON_WILD	||
	    a == ASON_TRUE	||
	    a == ASON_FALSE	||
	    a == ASON_OBJ_ANY)
		return a;

	a->refcount++;
	return a;
}

/**
 * Destroy an ASON value.
 **/
API_EXPORT void
ason_destroy(ason_t *a)
{
	size_t i;

	if (a == ASON_EMPTY	||
	    a == ASON_NULL	||
	    a == ASON_UNIVERSE	||
	    a == ASON_WILD	||
	    a == ASON_OBJ_ANY	||
	    a == ASON_TRUE	||
	    a == ASON_FALSE	||
	    --a->refcount)
		return;

	if (IS_OBJECT(a)) {
		for (i = 0; i < a->count; i++) {
			free(a->kvs[i].key);
			ason_destroy(a->kvs[i].value);
		}

		free(a->kvs);
	} else if (a->count > 0) {
		for (i = 0; i < a->count; i++)
			ason_destroy(a->items[i]);

		free(a->items);
	}

	free(a);
}

/**
 * Create an ASON numeric value.
 **/
API_EXPORT ason_t *
ason_create_number(int64_t number)
{
	return ason_create_fixnum(TO_FP(number));
}

/**
 * Create an ASON numeric value from a fixed point number.
 **/
ason_t *
ason_create_fixnum(int64_t number)
{
	ason_t *ret = ason_create(ASON_TYPE_NUMERIC, 0);

	ret->n = number;

	return ret;
}

/**
 * Create an ASON list value.
 **/
API_EXPORT ason_t *
ason_create_list(ason_t *content)
{
	ason_t *ret;

	if (! content)
		return ason_create(ASON_TYPE_LIST, 0);

	ret = ason_create(ASON_TYPE_LIST, 1);
	ret->items[0] = ason_copy(content);

	return ret;
}

/**
 * Create an ASON list with the values in one list as well as the values in
 * another list.
 **/
API_EXPORT ason_t *
ason_append_lists(ason_t *a, ason_t *b)
{
	ason_t *ret;
	size_t i;

	if (a->type != ASON_TYPE_LIST || b->type != ASON_TYPE_LIST)
		errx(1, "Arguments to ason_append_lists must be lists");

	ret = ason_create(ASON_TYPE_LIST, a->count + b->count);

	for (i = 0; i < a->count; i++)
		ret->items[i] = ason_copy(a->items[i]);

	for (i = 0; i < b->count; i++)
		ret->items[a->count + i] = ason_copy(b->items[i]);

	return ret;
}

/**
 * Create an ASON value.
 **/
API_EXPORT ason_t *
ason_create_object(const char *key, ason_t *value) 
{
	ason_t *ret;

	if (! value)
		return ason_create(ASON_TYPE_OBJECT, 0);

	if (value->type == ASON_TYPE_EMPTY)
		return ASON_EMPTY;

	ret = ason_create(ASON_TYPE_OBJECT, 1);

	ret->kvs[0].key = string_to_utf8(key);
	ret->kvs[0].value = ason_copy(value);

	return ret;
}

/**
 * Apply an operator to two ASON values.
 **/
static ason_t *
ason_operate(ason_t *a, ason_t *b, ason_type_t type)
{
	ason_t *ret;

	ret = ason_create(type, 2);

	ret->items[0] = ason_copy(a);
	ret->items[1] = ason_copy(b);

	return ret;
}

/**
 * Union two ASON values.
 **/
API_EXPORT ason_t *
ason_union(ason_t *a, ason_t *b)
{
	return ason_operate(a, b, ASON_TYPE_UNION);
}

/**
 * Intersect two ASON values.
 **/
API_EXPORT ason_t *
ason_intersect(ason_t *a, ason_t *b)
{
	return ason_operate(a, b, ASON_TYPE_INTERSECT);
}

/**
 * Join ASON value b to a.
 **/
API_EXPORT ason_t *
ason_join(ason_t *a, ason_t *b)
{
	return ason_operate(a, b, ASON_TYPE_JOIN);
}

/**
 * Complement an ASON value a.
 **/
API_EXPORT ason_t *
ason_complement(ason_t *a)
{
	ason_t *ret = ason_create(ASON_TYPE_COMP, 1);
	ret->items[0] = ason_copy(a);
	return ret;
}

/**
 * A boolean ASON value indicating whether a is represented in b.
 **/
API_EXPORT ason_t *
ason_representation_in(ason_t *a, ason_t *b)
{
	ason_t *ret = ason_create(ASON_TYPE_REPR, 2);
	ret->items[0] = ason_copy(a);
	ret->items[1] = ason_copy(b);
	return ret;
}

/**
 * A boolean ASON value indicating whether a is equal to b.
 **/
API_EXPORT ason_t *
ason_equality(ason_t *a, ason_t *b)
{
	ason_t *ret = ason_create(ASON_TYPE_EQUAL, 2);
	ret->items[0] = ason_copy(a);
	ret->items[1] = ason_copy(b);
	return ret;
}

/**
 * Convert an ASON value in place into an empty value.
 **/
static void
ason_make_empty(ason_t *a)
{
	size_t i;

	if (IS_OBJECT(a)) {
		for (i = 0; i < a->count; i++) {
			free(a->kvs[i].key);
			ason_destroy(a->kvs[i].value);
		}

		free(a->kvs);
		a->count = 0;
	}

	for (i = 0; i < a->count; i++)
		ason_destroy(a->items[i]);

	if (a->count)
		free(a->items);

	a->count = 0;
	a->type = ASON_TYPE_EMPTY;
	a->order = ORDER_OF_EMPTY;
}

/**
 * Alter the value of an ASON value to that of another value.
 **/
static void
ason_clone_into(ason_t *target, ason_t *src)
{
	size_t refcount = target->refcount;
	size_t i;

	src = ason_copy(src);
	ason_make_empty(target);

	memcpy(target, src, sizeof(ason_t));
	target->refcount = refcount;

	if (! target->count) {
		ason_destroy(src);
		return;
	}

	if (IS_OBJECT(target))
		target->kvs = xmemdup(target->kvs,
				      target->count * sizeof(struct kv_pair));
	else
		target->items = xmemdup(target->items,
					target->count * sizeof(ason_t *));

	for (i = 0; i < target->count; i++) {
		if (IS_OBJECT(target)) {
			target->kvs[i].key = xstrdup(target->kvs[i].key);
			target->kvs[i].value = ason_copy(target->kvs[i].value);
		} else {
			target->items[i] = ason_copy(target->items[i]);
		}
	}

	ason_destroy(src);
}

/**
 * Alter the value of an ASON value to that of another value.
 *
 * Then destroy that other value.
 **/
static void
ason_clone_into_d(ason_t *target, ason_t *src)
{
	ason_clone_into(target, src);
	ason_destroy(src);
}

/* Predeclaration */
int ason_reduce(ason_t *a);

/**
 * Reduce a complement.
 **/
static void
ason_reduce_complement(ason_t *a)
{
	ason_t *tmp;

	if (a->items[0]->type == ASON_TYPE_EMPTY) {
		ason_make_empty(a);
		a->type = ASON_TYPE_UNIVERSE;
		a->order = 2;
		return;
	} else if (a->items[0]->type == ASON_TYPE_UNIVERSE) {
		ason_make_empty(a);
		return;
	} else if (a->items[0]->type == ASON_TYPE_COMP) {
		tmp = ason_copy(a->items[0]->items[0]);
		ason_clone_into_d(a, tmp);
		return;
	}

	if (a->items[0]->order <= 1)
		a->order = 2;
	else
		a->order = 3;
}

/**
 * Distribute an ASON operator through a union. Return whether distribution was
 * possible.
 **/
static int
ason_distribute(ason_t *a)
{
	size_t source;
	size_t target;
	size_t i;
	ason_t *b;

	ason_t *right;
	ason_type_t type;

	if (a->items[0]->type == ASON_TYPE_UNION) {
		source = 1;
		target = 0;
	} else if (a->items[1]->type == ASON_TYPE_UNION) {
		source = 0;
		target = 1;
	} else {
		return 0;
	}

	right = ason_copy(a->items[source]);
	type = a->type;

	b = ason_copy(a->items[target]);
	ason_clone_into_d(a, b);

	for (i = 0; i < a->count; i++)
		a->items[i] = ason_operate(a->items[i], right, type);

	ason_destroy(right);
	a->order = ORDER_UNKNOWN;

	return 1;
}

/**
 * Reduce an intersect or join of two objects.
 **/
static void
ason_reduce_object_intersect_join(ason_t *a, int is_join)
{
	struct ason_coiterator iter;
	ason_t *left;
	ason_t *right;
	ason_t *tmp;
	const char *key;
	ason_type_t type = ASON_TYPE_OBJECT;
	struct kv_pair *buf = xcalloc(a->items[0]->count + a->items[1]->count,
				      sizeof(struct kv_pair));
	int max_order = 1;
	int order;

	if (a->items[0]->type == ASON_TYPE_UOBJECT &&
	    a->items[1]->type == ASON_TYPE_UOBJECT)
		type = ASON_TYPE_UOBJECT;

	if (is_join && a->items[0]->type == ASON_TYPE_UOBJECT)
		type = ASON_TYPE_UOBJECT;
	if (is_join && a->items[1]->type == ASON_TYPE_UOBJECT)
		type = ASON_TYPE_UOBJECT;

	if (type == ASON_TYPE_UOBJECT)
		max_order = 3;

	ason_coiterator_init(&iter, a->items[0], a->items[1]);
	ason_make_empty(a);
	a->type = type;
	a->kvs = buf;


	a->count = 0;

	while((key = ason_coiterator_next(&iter, &left, &right))) {
		if (is_join)
			tmp = ason_join(left, right);
		else
			tmp = ason_intersect(left, right);

		order = ason_reduce(tmp);

		if (order == ORDER_OF_EMPTY) {
			ason_destroy(tmp);
			ason_make_empty(a);
			ason_coiterator_release(&iter);
			return;
		}

		if (order > max_order)
			max_order = order;

		a->kvs[a->count].key = xstrdup(key);
		a->kvs[a->count].value = tmp;
		a->count++;
	}

	a->order = max_order;
	ason_coiterator_release(&iter);
}

/**
 * Reduce an intersect of two lists.
 **/
static void
ason_reduce_list_intersect(ason_t *a)
{
	ason_t *left;
	ason_t *right;
	ason_t *tmp;
	int max_order = 0;
	int order;

	if (a->items[0]->count != a->items[1]->count) {
		ason_make_empty(a);
		return;
	}

	left = ason_copy(a->items[0]);
	right = ason_copy(a->items[1]);
	ason_make_empty(a);

	a->type = ASON_TYPE_LIST;
	a->items = xcalloc(left->count, sizeof(ason_t *));

	for (a->count = 0; a->count < left->count; a->count++) {
		tmp = ason_intersect(left->items[a->count],
				     right->items[a->count]);

		order = ason_reduce(tmp);

		if (order == ORDER_OF_EMPTY) {
			ason_destroy(tmp);
			ason_make_empty(a);
			return;
		}

		if (order > max_order)
			max_order = order;

		a->items[a->count] = tmp;
	}

	a->order = max_order;
}

/**
 * Reduce an intersect.
 **/
static void
ason_reduce_intersect(ason_t *a)
{
	ason_type_t other;
	ason_t *tmp;
	char *string;
	int64_t n;

	if (a->items[0]->type == ASON_TYPE_EMPTY ||
	    a->items[1]->type == ASON_TYPE_EMPTY) {
		ason_make_empty(a);
	} else if (ason_distribute(a)) {
		ason_reduce(a);
	} else if (IS_OBJECT(a->items[0]) && IS_OBJECT(a->items[1])) {
		ason_reduce_object_intersect_join(a, 0);
	} else if (a->items[0]->type == ASON_TYPE_NULL) {
		other = a->items[1]->type;
		ason_make_empty(a);

		if (other == ASON_TYPE_NULL || other == ASON_TYPE_UNIVERSE) {
			a->type = ASON_TYPE_NULL;
			a->order = 0;
		}
	} else if (a->items[0]->type == ASON_TYPE_UNIVERSE) {
		ason_clone_into(a, a->items[1]);
	} else if (a->items[1]->type == ASON_TYPE_UNIVERSE) {
		ason_clone_into(a, a->items[0]);
	} else if (a->items[0]->type == ASON_TYPE_WILD &&
		   a->items[1]->type != ASON_TYPE_NULL) {
		ason_clone_into(a, a->items[1]);
	} else if (a->items[1]->type == ASON_TYPE_WILD &&
		   a->items[0]->type != ASON_TYPE_NULL) {
		ason_clone_into(a, a->items[0]);
	} else if (a->items[0]->type != a->items[1]->type) {
		if (IS_BOOL(a->items[0]) && IS_BOOL(a->items[1]))
			other = ASON_TYPE_FALSE;
		else
			other = ASON_TYPE_EMPTY;
		ason_make_empty(a);
		a->type = other;

		if (a->type != ASON_TYPE_EMPTY)
			a->order = 0;
	} else if (a->items[0]->type == ASON_TYPE_TRUE ||
		   a->items[0]->type == ASON_TYPE_FALSE) {
		other = a->items[0]->type;
		ason_make_empty(a);
		a->type = other;
		a->order = 0;
	} else if (a->items[0]->type == ASON_TYPE_STRING) {
		string = NULL;

		if (! strcmp(a->items[0]->string, a->items[1]->string))
			string = xstrdup(a->items[0]->string);

		ason_make_empty(a);

		if (string) {
			a->string = string;
			a->type = ASON_TYPE_STRING;
			a->order = 0;
		}
	} else if (a->items[0]->type == ASON_TYPE_NUMERIC) {
		n = a->items[1]->n;

		ason_clone_into(a, a->items[0]);

		if (a->n != n)
			ason_make_empty(a);
	} else if (a->items[0]->type == ASON_TYPE_LIST) {
		ason_reduce_list_intersect(a);
	} else if (a->items[0]->type == ASON_TYPE_COMP) {
		tmp = ason_union(a->items[0]->items[0], a->items[1]->items[0]);
		ason_make_empty(a);
		a->type = ASON_TYPE_COMP;
		a->count = 1;
		a->items = xmalloc(sizeof(ason_t *));
		a->items[0] = tmp;
		a->order = tmp->order;

		if (a->order < 2)
			a->order = 2;
	} else {
		ason_make_empty(a);
	}
}

/**
 * Reduce a join.
 **/
static void
ason_reduce_join(ason_t *a)
{
	ason_t *b;

	if (ason_distribute(a)) {
		ason_reduce(a);
		return;
	}

	if (a->items[0]->type == ASON_TYPE_NULL) {
		b = ason_copy(a->items[1]);
		ason_clone_into_d(a, b);
	} else if (a->items[1]->type == ASON_TYPE_NULL) {
		b = ason_copy(a->items[0]);
		ason_clone_into_d(a, b);
	} else if (IS_OBJECT(a->items[0]) && IS_OBJECT(a->items[1])) {
		ason_reduce_object_intersect_join(a, 1);
	} else {
		a->type = ASON_TYPE_INTERSECT;
		ason_reduce_intersect(a);
	}
}

/**
 * Reduce an object.
 **/
static void
ason_reduce_object(ason_t *a)
{
	size_t i;
	int max_order = 0;

	if (a->type == ASON_TYPE_UOBJECT)
		max_order = 3;

	for (i = 0; i < a->count; i++) {
		if (a->kvs[i].value->order > max_order)
			max_order = a->kvs[i].value->order;

		if (a->kvs[i].value->type != ASON_TYPE_EMPTY)
			continue;

		ason_make_empty(a);
		break;
	}

	a->order = max_order;
}

/**
 * Reduce a list.
 **/
static void
ason_reduce_list(ason_t *a)
{
	int ret = 0;
	size_t i;

	for (i = 0; !ret && i < a->count; i++) {
		if (a->items[i]->type != ASON_TYPE_EMPTY)
			continue;

		ason_make_empty(a);
		return;
	}
}

/**
 * Reduce a union.
 **/
static void
ason_reduce_union(ason_t *a)
{
	size_t new_count;
	size_t pos;
	int found_uni = 0;
	int found_wild = 0;
	int found_null = 0;
	int max_order = 1;

	for (pos = new_count = 0; pos < a->count; pos++) {
		if (a->items[pos]->order > max_order)
			max_order = a->items[pos]->order;

		if (a->items[pos]->type != ASON_TYPE_EMPTY) {
			a->items[new_count++] = a->items[pos];
		} else {
			ason_destroy(a->items[pos]);
		}
	}

	a->count = new_count;

	for (pos = 0; pos < a->count; pos++) {
		if (a->items[pos]->type == ASON_TYPE_UNIVERSE) {
			found_uni = 1;
		} else if (a->items[pos]->type == ASON_TYPE_WILD) {
			found_wild = 1;
		} else if (a->items[pos]->type == ASON_TYPE_NULL) {
			found_null = 1;
		}
	}

	found_uni = found_uni || (found_null && found_wild);

	if (found_uni) {
		ason_make_empty(a);
		a->type = ASON_TYPE_UNIVERSE;
		a->order = 2;
		return;
	} else if (! new_count) {
		ason_make_empty(a);
		return;
	}

	a->order = max_order;
}

/**
 * Reduce an ASON value. This technically mutates the value, but our API allows
 * values to change representation as long as they remain equal to themselves.
 *
 * Return the order of the value.
 **/
int
ason_reduce(ason_t *a)
{
	ason_t *tmp;
	size_t i;

	if (a->type == ASON_TYPE_EMPTY)
		a->order = ORDER_OF_EMPTY;

	if (a->order != ORDER_UNKNOWN)
		return a->order;

	if (! a->count) {
		if (a->type == ASON_TYPE_UOBJECT)
			a->order = 3;
		else if (a->type == ASON_TYPE_EMPTY)
			a->order = ORDER_OF_EMPTY;
		else if (a->type == ASON_TYPE_UNIVERSE ||
			 a->type == ASON_TYPE_WILD)
			a->order = 2;
		else
			a->order = 0;

		return a->order;
	}

	for (i = 0; i < a->count; i++) {
		if (IS_OBJECT(a))
			ason_reduce(a->kvs[i].value);
		else
			ason_reduce(a->items[i]);
	}


	if (a->type == ASON_TYPE_EQUAL) {
		tmp = ason_representation_in(a->items[0], a->items[1]);
		ason_reduce(tmp);

		if (tmp->type == ASON_TYPE_TRUE) {
			ason_destroy(tmp);
			tmp = ason_representation_in(a->items[1], a->items[0]);
			ason_reduce(tmp);
		}

		ason_make_empty(a);
		a->type = tmp->type;
		a->order = tmp->order;
		ason_destroy(tmp);
		return a->order;
	}

	if (a->type == ASON_TYPE_REPR) {
		a->type = ASON_TYPE_INTERSECT;
		a->items[1] = ason_complement_d(a->items[1]);

		if (ason_reduce(a) == ORDER_OF_EMPTY)
			a->type = ASON_TYPE_TRUE;
		else
			a->type = ASON_TYPE_FALSE;

		a->order = 0;
		return a->order;
	}

	if (IS_OBJECT(a))
		ason_reduce_object(a);
	else if (a->type == ASON_TYPE_LIST)
		ason_reduce_list(a);
	else if (a->type == ASON_TYPE_UNION)
		ason_reduce_union(a);
	else if (a->type == ASON_TYPE_INTERSECT)
		ason_reduce_intersect(a);
	else if (a->type == ASON_TYPE_JOIN)
		ason_reduce_join(a);
	else if (a->type == ASON_TYPE_COMP)
		ason_reduce_complement(a);

	return a->order;
}

/**
 * Check intersectionality of two ASON values.
 **/
API_EXPORT int
ason_check_intersects(ason_t *a, ason_t *b)
{
	ason_t *inter = ason_intersect(a, b);
	int ret = !ason_reduce(inter);

	ason_destroy(inter);
	return ret;
}

/**
 * Check whether ASON value a is represented in b.
 **/
API_EXPORT int
ason_check_represented_in(ason_t *a, ason_t *b)
{
	ason_t *inter = ason_representation_in(a, b);
	int ret;

	ason_reduce(inter);
	ret = inter->type == ASON_TYPE_TRUE;

	ason_destroy(inter);
	return ret;
}

/**
 * Check whether a and b are equal.
 **/
API_EXPORT int
ason_check_equal(ason_t *a, ason_t *b)
{
	ason_t *inter = ason_equality(a, b);
	int ret;

	ason_reduce(inter);
	ret = inter->type == ASON_TYPE_TRUE;

	ason_destroy(inter);
	return ret;
}
