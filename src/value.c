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

/* Macros to get a value's child. */
#define CHILD_VALUE_ADDR(_a, _idx) ({ \
	ason_t *a__ = (_a); \
	size_t idx__ = (_idx); \
	ason_t **ret__; \
	if (IS_OBJECT(a__)) \
		ret__ = &a__->kvs[idx__].value; \
	else \
		ret__ = &a__->items[idx__]; \
	ret__; \
})

#define CHILD_VALUE(_a, _idx) (*(CHILD_VALUE_ADDR(_a, _idx)))

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
	.type = ASON_TYPE_COMP,
	.items = (ason_t **)&ASON_EMPTY,
	.count = 1,
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
	.type = ASON_TYPE_COMP,
	.items = (ason_t **)&ASON_NULL,
	.count = 1,
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
 * Remove some items from an ASON object. This is generally a mutating operation
 * so it must be done with care so as not to break value-immutability.
 **/
static void
ason_remove_items(ason_t *a, size_t idx, size_t count, int destroy_removed)
{
	size_t i;
	size_t stop_idx = idx + count;

	if (IS_OBJECT(a))
		errx(1, "Tried to remove items from value with KV pairs");

	if (idx >= a->count)
		errx(1, "Tried to remove item that was not present");

	if (stop_idx > a->count)
		errx(1, "Tried to remove item that was not present");

	for (i = idx; i < stop_idx && destroy_removed; i++)
		ason_destroy(a->items[i]);

	memmove(&a->items[idx], &a->items[stop_idx],
		(a->count - stop_idx) * sizeof(ason_t *));
	a->count -= count;
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

	if (! a)
		return;

	if (a == ASON_EMPTY	||
	    a == ASON_NULL	||
	    a == ASON_UNIVERSE	||
	    a == ASON_WILD	||
	    a == ASON_OBJ_ANY	||
	    a == ASON_TRUE	||
	    a == ASON_FALSE	||
	    --a->refcount)
		return;

	if (a->type == ASON_TYPE_STRING) {
		free(a->string);
	} else if (IS_OBJECT(a)) {
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
 * Compare two ASON values. Beyond equality, the ordering doesn't mean much,
 * but is total, which is useful. Returns an integer a la strcmp.
 **/
static int
ason_compare(ason_t *a, ason_t *b)
{
	int ret;
	size_t i;

	ason_reduce(a);
	ason_reduce(b);

	if (a->order != b->order)
		return a->order - b->order;

	if (a->type != b->type)
		return a->type - b->type;

	if (a->count != b->count)
		return a->count - b->count;

	switch (a->type) {
	case ASON_TYPE_EMPTY:
	case ASON_TYPE_NULL:
	case ASON_TYPE_TRUE:
	case ASON_TYPE_FALSE:
		return 0;

	case ASON_TYPE_NUMERIC:
		return a->n - b->n;

	case ASON_TYPE_STRING:
		return strcmp(a->string, b->string);

	case ASON_TYPE_COMP:
		return ason_compare(a->items[0], b->items[0]);

	case ASON_TYPE_UNION:
	case ASON_TYPE_LIST:
		ret = 0;

		for (i = 0; i < a->count && ! ret; i++)
			ret = ason_compare(a->items[i], b->items[i]);

		return ret;

	case ASON_TYPE_OBJECT:
	case ASON_TYPE_UOBJECT:
		for (i = 0; i < a->count; i++) {
			ret = strcmp(a->kvs[i].key, b->kvs[i].key);

			if (ret)
				return ret;

			ret = ason_compare(a->kvs[i].value, b->kvs[i].value);

			if (ret)
				return ret;
		}

		return 0;

	case ASON_TYPE_REPR:
	case ASON_TYPE_EQUAL:
	case ASON_TYPE_JOIN:
	case ASON_TYPE_INTERSECT:
		errx(1, "Unsimplified type in reduced object");
	default:
		errx(1, "Unknown type");
	}
}

/**
 * Reduce a complement.
 **/
static void
ason_reduce_complement(ason_t *a)
{
	if (a->items[0]->type == ASON_TYPE_COMP)
		ason_clone_into(a, a->items[0]->items[0]);
	else if (a->items[0]->order <= 1)
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

	ason_clone_into(a, a->items[target]);

	for (i = 0; i < a->count; i++) {
		b = a->items[i];
		a->items[i] = ason_operate(a->items[i], right, type);
		ason_destroy(b);
	}

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
	int max_order = 0;
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

	if (max_order > 1)
		max_order = 3;

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
			ason_destroy(left);
			ason_destroy(right);
			return;
		}

		if (order > max_order)
			max_order = order;

		a->items[a->count] = tmp;
	}

	ason_destroy(left);
	ason_destroy(right);

	if (max_order > 1)
		max_order = 3;

	a->order = max_order;
}

/**
 * Turn a union of zero or one values into a simpler value. Return true if the
 * change was made.
 **/
static int
ason_union_collapse(ason_t *a)
{
	if (a->count > 1)
		return 0;

	if (a->count)
		ason_clone_into(a, a->items[0]);
	else
		ason_make_empty(a);

	return 1;
}

/**
 * Reduce an intersect of two order 3 values of which a is a collection and b
 * is a complement of a list.
 **/
static void
ason_reduce_intersect_col3_comp3_list(ason_t *a)
{
	ason_t **results;
	ason_t *tmp;
	size_t i;
	size_t count;

	if (a->items[0]->count != a->items[1]->items[0]->count) {
		ason_clone_into_d(a, a->items[0]);
		return;
	}

	count = a->items[0]->count;
	results = xcalloc(count, sizeof(ason_t *));

	for (i = 0; i < count; i++) {
		results[i] = ason_create(ASON_TYPE_EMPTY, 0);
		ason_clone_into(results[i], a->items[0]);
		results[i]->order = ORDER_UNKNOWN;
		tmp = ason_complement(a->items[1]->items[0]->items[i]);
		tmp = ason_intersect_d(results[i]->items[i], tmp);
		results[i]->items[i] = tmp;
	}

	ason_make_empty(a);
	a->type = ASON_TYPE_UNION;
	a->count = count;
	a->items = results;
	a->order = ORDER_UNKNOWN;
}

/**
 * Reduce an intersect of two order 3 values of which a is a collection and b
 * is a complement of an object.
 **/
static void
ason_reduce_intersect_col3_comp3_object(ason_t *a)
{
	ason_t **results;
	ason_t *tmp;
	ason_t *left;
	ason_t *right;
	size_t i;
	struct ason_coiterator iter;
	const char *key;

	results = xcalloc(a->items[0]->count +
			  a->items[1]->items[0]->count, sizeof(ason_t *));

	ason_coiterator_init(&iter, a->items[0], a->items[1]->items[0]);

	for (i = 0; (key = ason_coiterator_next(&iter, &left, &right)); i++) {
		results[i] = ason_create(ASON_TYPE_UOBJECT, 1);
		results[i]->kvs[0].key = xstrdup(key);
		tmp = ason_complement(right);
		results[i]->kvs[0].value = ason_intersect(left, tmp);
		ason_destroy(tmp);
	}

	results = xrealloc(results, i * sizeof(ason_t *));

	tmp = ason_create(ASON_TYPE_UNION, 0);
	tmp->count = i;
	tmp->items = results;
	tmp->order = ORDER_UNKNOWN;

	ason_destroy(a->items[1]);
	a->items[1] = tmp;
	a->order = ORDER_UNKNOWN;
}

/**
 * Reduce an intersect of two order 3 values of which a is a collection and b
 * is a complement of a union.
 **/
static void
ason_reduce_intersect_col3_comp3_union(ason_t *a)
{
	ason_t *tmp;

	while (a->items[1]->items[0]->type == ASON_TYPE_UNION) {
		a->items[1]->items[0]->order = ORDER_UNKNOWN;
		a->items[1]->order = ORDER_UNKNOWN;

		tmp = ason_complement_d(a->items[1]->items[0]->items[0]);
		ason_remove_items(a->items[1]->items[0], 0, 1, 0);

		a->items[0] = ason_intersect_d(a->items[0], tmp);

		ason_union_collapse(a->items[1]->items[0]);
	}
}

/**
 * Reduce an intersect of two order 3 values of which a is a collection and b
 * is a complementation.
 **/
static void
ason_reduce_intersect_col3_comp3(ason_t *a)
{
	ason_t *tmp;

	if (a->items[0]->type == ASON_TYPE_COMP) {
		tmp = a->items[0];
		a->items[0] = a->items[1];
		a->items[1] = tmp;
	}

	if (a->items[1]->items[0]->type == ASON_TYPE_UNION)
		ason_reduce_intersect_col3_comp3_union(a);
	else if (IS_OBJECT(a->items[0]) && IS_OBJECT(a->items[1]->items[0]))
		ason_reduce_intersect_col3_comp3_object(a);
	else if (a->items[0]->type == ASON_TYPE_LIST &&
		 a->items[1]->items[0]->type == ASON_TYPE_LIST)
		ason_reduce_intersect_col3_comp3_list(a);
	else
		ason_clone_into_d(a, a->items[0]);
}

/**
 * Reduce an intersect.
 **/
static void
ason_reduce_intersect(ason_t *a)
{
	ason_t *tmp;

	if (a->items[0]->type == ASON_TYPE_EMPTY ||
	    a->items[1]->type == ASON_TYPE_EMPTY) {
		ason_make_empty(a);
		return;
	}

	/* The more complex object will always be on the right */
	if (a->items[0]->order > a->items[1]->order) {
		tmp = a->items[0];
		a->items[0] = a->items[1];
		a->items[1] = tmp;
	}

	if (a->items[0]->order == 0) {
		if (ason_check_represented_in(a->items[0], a->items[1]))
			ason_clone_into(a, a->items[0]);
		else
			ason_make_empty(a);

		return;
	}

	if (ason_distribute(a))
		return;

	/* Distribute took care of order 1 objects. We cleared up order 0
	 * objects earlier. Everything left is order 2 or 3.
	 */

	if (a->items[1]->order == 2 || (a->items[0]->type == ASON_TYPE_COMP &&
					a->items[1]->type == ASON_TYPE_COMP)) {
		a->items[0] = ason_complement_d(a->items[0]);
		a->items[1] = ason_complement_d(a->items[1]);
		tmp = ason_union(a->items[0], a->items[1]);
		ason_make_empty(a);
		a->type = ASON_TYPE_COMP;
		a->count = 1;
		a->items = xmalloc(sizeof(ason_t *));
		a->items[0] = tmp;
		a->order = ORDER_UNKNOWN;
		return;
	}

	/* At least one of the parameters is now order 3. God help us. */

	if (IS_OBJECT(a->items[0]) && IS_OBJECT(a->items[1])) {
		ason_reduce_object_intersect_join(a, 0);
	} else if (a->items[0]->type == ASON_TYPE_LIST &&
		   a->items[1]->type == ASON_TYPE_LIST) {
		ason_reduce_list_intersect(a);
	} else {
		/* The case remaining is: one of these is a complement, the
		 * other is an object or list
		 */
		ason_reduce_intersect_col3_comp3(a);
	}
}

/**
 * Reduce a join.
 **/
static void
ason_reduce_join(ason_t *a)
{
	ason_t *b;

	if (ason_distribute(a))
		return;

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
 * Split an object or list with union elements into many objects or lists with
 * non-union elements.
 **/
static void
ason_splay(ason_t *a)
{
	size_t i,j;
	size_t union_count = 0;
	size_t curr_union;
	size_t results_count = 1;
	ason_t **results;
	size_t *positions;

	for (i = 0; i < a->count; i++) {
		if (CHILD_VALUE(a, i)->type != ASON_TYPE_UNION)
			continue;

		union_count++;
		results_count *= CHILD_VALUE(a, i)->count;
	}

	if (! union_count)
		return;

	positions = xcalloc(union_count, sizeof(size_t));
	results = xcalloc(results_count, sizeof(ason_t *));

	for (i = 0; i < results_count; i++) {
		results[i] = ason_create(ASON_TYPE_EMPTY, 0);
		ason_clone_into(results[i], a);
		results[i]->order = ORDER_UNKNOWN;
	}

	for (i = 0; i < results_count; i++) {
		curr_union = 0;

		for (j = 0; j < a->count; j++) {
			if (CHILD_VALUE(a, j)->type != ASON_TYPE_UNION)
				continue;
			ason_destroy(CHILD_VALUE(results[i], j));
			CHILD_VALUE(results[i], j) =
				ason_copy(CHILD_VALUE(a, j)->items[positions[
					  curr_union++]]);
		}

		curr_union = 0;

		for (j = 0; j < a->count; j++) {
			if (CHILD_VALUE(a, j)->type != ASON_TYPE_UNION)
				continue;

			positions[curr_union]++;

			if (CHILD_VALUE(a, j)->count > positions[curr_union])
				break;

			positions[curr_union++] = 0;
		}
	}

	free(positions);
	ason_make_empty(a);
	a->type = ASON_TYPE_UNION;
	a->items = results;
	a->count = results_count;
	a->order = ORDER_UNKNOWN;
}

/**
 * Reduce an object or list.
 **/
static void
ason_reduce_collection(ason_t *a)
{
	size_t i;
	int max_order = 0;

	if (a->type == ASON_TYPE_UOBJECT)
		max_order = 3;

	for (i = 0; i < a->count; i++) {
		if (CHILD_VALUE(a, i)->order > max_order)
			max_order = CHILD_VALUE(a, i)->order;

		if (CHILD_VALUE(a, i)->type != ASON_TYPE_EMPTY)
			continue;

		ason_make_empty(a);
		return;
	}

	if (max_order > 1)
		max_order = 3;

	a->order = max_order;
	ason_splay(a);
}

/**
 * Sort the items in a union.
 **/
static void
ason_union_sort(ason_t *a, size_t start, size_t end)
{
	size_t i;
	size_t pivot = start;
	ason_t *tmp;

	if (end - start <= 1)
		return;

	for (i = start + 1; i < end;) {
		if (ason_compare(a->items[pivot], a->items[i]) <= 0) {
			i++;
			continue;
		}

		if (i == (pivot + 1)) {
			tmp = a->items[i];
			a->items[i] = a->items[pivot];
			a->items[pivot] = tmp;
		} else {
			tmp = a->items[pivot + 1];
			a->items[pivot + 1] = a->items[pivot];
			a->items[pivot] = a->items[i];
			a->items[i] = tmp;
		}

		pivot++;
		if (pivot == i)
			i++;
	}

	ason_union_sort(a, start, pivot);
	ason_union_sort(a, pivot + 1, end);
}

/**
 * Turn a union of unions into a single union, or otherwise promote children in
 * union members of unions.
 **/
static void
ason_union_make_level(ason_t *a)
{
	size_t i;
	size_t j;
	ason_t *sub;

	for (i = 0; i < a->count; i++) {
		sub = a->items[i];

		if (sub->type != ASON_TYPE_UNION)
			continue;

		a->items = xrealloc(a->items, (a->count + sub->count) *
				    sizeof(ason_t *));

		memmove(&a->items[i + sub->count], &a->items[i + 1],
			(a->count - i - 1) * sizeof(ason_t *));

		for (j = 0; j < sub->count; j++)
			a->items[i + j] = ason_copy(sub->items[j]);

		a->count += sub->count - 1;
		i += sub->count - 1;

		ason_destroy(sub);
	}
}

/**
 * Reduce the leading items of a union when they are, respectively, a series of
 * order 0 items and an order 2 item.
 **/
static void
ason_reduce_union_0_2(ason_t *a, size_t o2_idx)
{
	ason_t **values = xcalloc(o2_idx, sizeof(ason_t *));
	size_t i;
	size_t j;
	ason_t *other;

	values = memcpy(values, a->items, o2_idx * sizeof(ason_t *));
	ason_remove_items(a, 0, o2_idx, 0);

	if (ason_union_collapse(a))
		other = a->items[0];
	else
		other = a->items[0]->items[0];

	if (other->type != ASON_TYPE_UNION) {
		for (i = 0; i < o2_idx; i++)
			if (ason_check_equal(other, values[i]))
				break;

		if (i != o2_idx)
			ason_make_empty(other);
	} else {
		for (i = 0; i < o2_idx; i++) {
			for (j = 0; j < other->count; j++)
				if (ason_check_equal(values[i],
						     other->items[j]))
					break;

			if (j == other->count)
				continue;

			ason_remove_items(other, j, 1, 1);
		}

		ason_union_collapse(other);
	}

	for (i = 0; i < o2_idx; i++)
		ason_destroy(values[i]);

	free(values);
}

/**
 * Reduce a union of zero or more order 0 values and one or more order 3
 * values.
 **/
static void
ason_reduce_union_0_3(ason_t *a)
{
	size_t i;
	size_t j;
	size_t hits = 0;
	ason_t *tmp;
	int rereduce = 0;

	for (i = 0; a->items[i]->order != 3; i++);

	while ((i + 1) < a->count) {
		if (a->items[i]->order != 3)
			return;

		if (a->items[i]->type != ASON_TYPE_COMP) {
			i++;
			continue;
		}

		if (a->items[i + 1]->type != ASON_TYPE_COMP) {
			i++;
			continue;
		}

		tmp = ason_intersect_d(ason_complement_d(a->items[i]),
				       ason_complement_d(a->items[i + 1]));

		tmp = ason_complement_d(tmp);

		ason_remove_items(a, i, 1, 0);
		a->items[i] = tmp;
		hits++;
	}

	for (i = 0; a->items[i]->order != 3 && i < a->count; i++);

	if (i == a->count)
		rereduce = 1;

	for (; i < a->count - 1; i++) {
		for (j = i + 1; j < a->count; j++) {
			tmp = ason_complement(a->items[j]);
			a->items[i] = ason_intersect_d(a->items[i], tmp);
			ason_reduce(a->items[i]);

			if (a->items[i]->order != 3)
				rereduce = 1;
		}
	}

	if (rereduce)
		return;

	ason_union_sort(a, 0, a->count);
	a->order = 3;
}
/**
 * Reduce a union.
 **/
static void
ason_reduce_union(ason_t *a)
{
	size_t i;
	size_t j;
	size_t k;
	ason_t *tmp;

	ason_union_make_level(a);
	ason_union_sort(a, 0, a->count);

	for (i = 0; i < a->count; ) {
		if (a->items[i]->order >= 2)
			break;

		if (a->items[i]->type != ASON_TYPE_EMPTY &&
		    ( i == 0 || ason_compare(a->items[i - 1], a->items[i]))) {
			i++;
			continue;
		}

		ason_remove_items(a,i,1,1);
	}

	if (ason_union_collapse(a))
		return;

	if (i == a->count) {
		a->order = 1;
		return;
	}

	while (i < (a->count - 1) && a->items[i + 1]->order == 2) {
		tmp = ason_intersect(a->items[i]->items[0],
				     a->items[i + 1]->items[0]);
		ason_reduce(tmp);
		ason_destroy(a->items[i]->items[0]);
		a->items[i]->items[0] = tmp;
		ason_reduce(a->items[i]);
		ason_remove_items(a,i + 1,1,1);
	}

	if (ason_union_collapse(a))
		return;

	/* i is the index of the first value where order is not 0 */
	for (j = 0; j < i;) {
		for (k = i; k < a->count; k++)
			if (ason_check_represented_in(a->items[j],
						      a->items[k]))
				break;

		if (k == a->count) {
			j++;
			continue;
		}

		ason_remove_items(a,j,1,1);
		i--;
	}

	if (ason_union_collapse(a))
		return;

	if (a->items[0]->order == 0 && a->items[i]->order == 2)
		ason_reduce_union_0_2(a, i);

	if (a->type != ASON_TYPE_UNION)
		return;

	if (a->items[0]->order != 2) {
		ason_reduce_union_0_3(a);
		return;
	}

	tmp = a->items[0]->items[0];

	if (! tmp->order) {
		for (j = 1; j < a->count; j++) {
			if (! ason_check_represented_in(tmp, a->items[j]))
				continue;

			ason_make_empty(tmp);
		}

		ason_clone_into(a, a->items[0]);
		return;
	}

	for (i = 0; i < tmp->count;) {
		for (j = 1; j < a->count; j++)
			if (ason_check_represented_in(tmp->items[i],
						      a->items[j]))
				break;

		if (j == a->count) {
			i++;
			continue;
		}

		ason_remove_items(tmp,i,1,1);
	}

	if (! tmp->count)
		ason_make_empty(tmp);

	ason_clone_into(a, a->items[0]);
}

/**
 * A stack of ason_t * values and their position within their parents;
 **/
struct reduce_stack {
	ason_t *item;
	size_t pos;
	struct reduce_stack *next;
};

/**
 * Push a reduce_stack.
 **/
static void
reduce_stack_push(struct reduce_stack **stack, ason_t *item, size_t pos)
{
	struct reduce_stack *frame = xmalloc(sizeof(struct reduce_stack));

	frame->next = *stack;
	frame->item = item;
	frame->pos = pos;
	*stack = frame;
}

/**
 * Pop reduce_stack.
 **/
static ason_t *
reduce_stack_pop(struct reduce_stack **stack, size_t *pos)
{
	ason_t *ret;
	struct reduce_stack *frame = *stack;

	if (! *stack)
		return NULL;

	ret = frame->item;
	*stack = frame->next;
	*pos = frame->pos;
	free(frame);

	return ret;
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
	size_t i;
	struct reduce_stack *stack = NULL;

restart:
	if (a->type == ASON_TYPE_EMPTY)
		a->order = ORDER_OF_EMPTY;

	if (a->order != ORDER_UNKNOWN)
		goto out;

	if (! a->count) {
		if (a->type == ASON_TYPE_UOBJECT)
			a->order = 3;
		else if (a->type == ASON_TYPE_EMPTY)
			a->order = ORDER_OF_EMPTY;
		else
			a->order = 0;

		goto out;
	}

	for (i = 0; i < a->count; i++) {
		if (CHILD_VALUE(a, i)->order != ORDER_UNKNOWN)
			continue;

		reduce_stack_push(&stack, a, i);
		a = CHILD_VALUE(a, i);
		goto restart;
next_child:
		;
	}

	if (a->type == ASON_TYPE_EQUAL) {
		if (ason_check_equal(a->items[0], a->items[1]))
			ason_clone_into(a, ASON_TRUE);
		else
			ason_clone_into(a, ASON_FALSE);

		goto out;
	}

	if (a->type == ASON_TYPE_REPR) {
		if (ason_check_represented_in(a->items[0], a->items[1]))
			ason_clone_into(a, ASON_TRUE);
		else
			ason_clone_into(a, ASON_FALSE);

		goto out;
	}

	if (IS_OBJECT(a) || a->type == ASON_TYPE_LIST)
		ason_reduce_collection(a);
	else if (a->type == ASON_TYPE_UNION)
		ason_reduce_union(a);
	else if (a->type == ASON_TYPE_INTERSECT)
		ason_reduce_intersect(a);
	else if (a->type == ASON_TYPE_JOIN)
		ason_reduce_join(a);
	else if (a->type == ASON_TYPE_COMP)
		ason_reduce_complement(a);

	if (a->order == ORDER_UNKNOWN)
		goto restart;

out:
	if (! stack) {
		return a->order;
	} else {
		a = reduce_stack_pop(&stack, &i);
		goto next_child;
	}
}

/**
 * Check whether ASON value a is represented in b, where both are objects.
 **/
static int
ason_check_object_represented_in(ason_t *a, ason_t *b)
{
	struct ason_coiterator iter;
	int ret = 1;
	ason_t *left;
	ason_t *right;

	if (a->type == ASON_TYPE_UOBJECT && b->type == ASON_TYPE_OBJECT)
		return 0;

	ason_coiterator_init(&iter, a, b);

	while (ason_coiterator_next(&iter, &left, &right)) {
		if (ason_check_represented_in(left, right))
			continue;

		ret = 0;
		break;
	}

	ason_coiterator_release(&iter);
	return ret;
}

/**
 * Check whether ASON value a is represented in b, where both are lists.
 **/
static int
ason_check_list_represented_in(ason_t *a, ason_t *b)
{
	size_t i;

	if (a->count != b->count)
		return 0;

	for (i = 0; i < a->count; i++)
		if (! ason_check_represented_in(a->items[i], b->items[i]))
			return 0;

	return 1;
}

/**
 * Check whether ASON value a is represented in b.
 **/
API_EXPORT int
ason_check_represented_in(ason_t *a, ason_t *b)
{
	size_t i;
	int ret;
	ason_t *tmp;

	ason_reduce(a);
	ason_reduce(b);

	if (a->order > b->order) {
		if (a->order != 3)
			return 0;
		if (b->order != 2)
			return 0;
	}

	if (a->order == ORDER_OF_EMPTY)
		return 1;
	if (b->order == ORDER_OF_EMPTY)
		return 0;

	if (b->order == 0)
		return ason_check_equal(a, b);

	if (a->type == ASON_TYPE_UNION) {
		for (i = 0; i < a->count; i++)
			if (! ason_check_represented_in(a->items[i], b))
				return 0;
		return 1;
	}

	if (b->order == 1 || (a->order == 0 &&
			      b->type == ASON_TYPE_UNION)) {
		for (i = 0; i < b->count; i++)
			if (ason_check_equal(a, b->items[i]))
				return 1;
		return 0;
	}

	if (b->order == 2) {
		if (b->items[0]->order == ORDER_OF_EMPTY)
			return 1;

		if (b->items[0]->order == 0)
			return ! ason_check_represented_in(b->items[0], a);

		for (i = 0; i < b->items[0]->count; i++)
			if (ason_check_represented_in(b->items[0]->items[i],
						      a))
				return 0;

		return 1;
	}

	/* b->order == 3 */
	if (b->order != 3)
		errx(1, "Unknown order in ason_check_represented_in");

	if (a->order == 2)
		return 0;

	if (IS_OBJECT(a) && IS_OBJECT(b))
		return ason_check_object_represented_in(a, b);
	if (a->type == ASON_TYPE_LIST && b->type == ASON_TYPE_LIST)
		return ason_check_list_represented_in(a, b);

	if (a->order == 0 && b->type == ASON_TYPE_COMP)
		return ! ason_check_represented_in(a, b->items[0]);

	/* We've handled every other order 0 case for a */
	if (a->order == 0)
		return 0;

	/* a->order == 3 */

	/* We know a->type != ASON_TYPE_UNION */
	if (b->type != ASON_TYPE_UNION &&
	    a->type != ASON_TYPE_COMP && b->type != ASON_TYPE_COMP)
		return 0;

	if (a->type == ASON_TYPE_COMP && b->type == ASON_TYPE_COMP) {
		a = ason_complement(a);
		b = ason_complement(b);

		ret = ason_check_represented_in(b, a);

		ason_destroy(a);
		ason_destroy(b);

		return ret;
	}

	if (IS_OBJECT(b))
		return 0;
	if (b->type == ASON_TYPE_LIST)
		return 0;

	/* This /should/ work as long as we still splay values. I'm nervous
	 * about missing cases here though. Specifically we're worried about a
	 * case where a is represented partially by one term of b and partially
	 * by another, but I can't construct an example that isn't handled
	 * elswhere or taken apart by splay.
	 */
	if (b->type == ASON_TYPE_UNION) {
		for (i = 0; i < b->count; i++)
			if (ason_check_represented_in(a, b->items[i]))
				return 1;

		return 0;
	}

	if (b->type != ASON_TYPE_COMP)
		errx(1, "Unknown case in ason_check_represented_in");

	tmp = ason_intersect(a, b->items[0]);
	ret = ason_check_equal(tmp, ASON_EMPTY);
	ason_destroy(tmp);

	return ret;
}

/**
 * Check whether a and b are equal.
 **/
API_EXPORT int
ason_check_equal(ason_t *a, ason_t *b)
{
	return !ason_compare(a, b);
}
