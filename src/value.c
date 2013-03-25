/**
 * This file is part of libasonalg.
 *
 * libasonalg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libasonalg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libasonalg. If not, see <http://www.gnu.org/licenses/>.
 **/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <err.h>

#include <ason/value.h>

#include "value.h"


/**
 * Handy value constants.
 **/
static struct ason VALUE_NULL_DATA = {
	.type = ASON_NULL,
	.items = NULL,
	.count = 0,
};
ason_t * const VALUE_NULL = &VALUE_NULL_DATA;

static struct ason VALUE_STRONG_NULL_DATA = {
	.type = ASON_STRONG_NULL,
	.items = NULL,
	.count = 0,
};
ason_t * const VALUE_STRONG_NULL = &VALUE_STRONG_NULL_DATA;

static struct ason VALUE_UNIVERSE_DATA = {
	.type = ASON_UNIVERSE,
	.items = NULL,
	.count = 0,
};
ason_t * const VALUE_UNIVERSE = &VALUE_UNIVERSE_DATA;

static struct ason VALUE_WILD_DATA = {
	.type = ASON_WILD,
	.items = NULL,
	.count = 0,
};
ason_t * const VALUE_WILD = &VALUE_WILD_DATA;

static struct ason VALUE_OBJ_ANY_DATA = {
	.type = ASON_UOBJECT,
	.items = NULL,
	.count = 0,
};
ason_t * const VALUE_OBJ_ANY = &VALUE_OBJ_ANY_DATA;

/**
 * Create a new ASON value struct.
 **/
static ason_t *
ason_create(ason_type_t type, size_t count, int use_kvs)
{
	ason_t *ret = xcalloc(1, sizeof(struct ason));

	ret->type = type;
	ret->count = count;
	ret->refcount = 1;

	if (! ret->count)
		return ret;

	if (use_kvs)
		ret->kvs = xcalloc(count, sizeof(struct kv_pair));
	else
		ret->items = xcalloc(count, sizeof(ason_t *));

	return ret;
}

/**
 * Copy an ASON value.
 **/
ason_t *
ason_copy(ason_t *a)
{
	if (a == VALUE_NULL		||
	    a == VALUE_STRONG_NULL	||
	    a == VALUE_UNIVERSE		||
	    a == VALUE_WILD		||
	    a == VALUE_OBJ_ANY)
		return a;

	a->refcount++;
	return a;
}

/**
 * Destroy an ASON value.
 **/
void
ason_destroy(ason_t *a)
{
	size_t i;

	if (a == VALUE_NULL		||
	    a == VALUE_STRONG_NULL	||
	    a == VALUE_UNIVERSE		||
	    a == VALUE_WILD		||
	    a == VALUE_OBJ_ANY		||
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
ason_t *
ason_create_number(int number)
{
	ason_t *ret = ason_create(ASON_NUMERIC, 0, 0);

	ret->n = number;

	return ret;
}

/**
 * Create an ASON list value.
 **/
ason_t *
ason_create_list(ason_t *content)
{
	ason_t *ret;

	if (! content)
		return ason_create(ASON_LIST, 0, 0);

	ret = ason_create(ASON_LIST, 1, 0);
	ret->items[0] = ason_copy(content);

	return ret;
}

/**
 * Create an ASON value.
 **/
ason_t *
ason_create_object(const char *key, ason_t *value) 
{
	ason_t *ret;

	if (! value)
		return ason_create(ASON_OBJECT, 0, 0);

	ret = ason_create(ASON_OBJECT, 1, 1);

	ret->kvs[0].key = xstrdup(key);
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

	ret = ason_create(type, 2, 0);

	ret->items[0] = ason_copy(a);
	ret->items[1] = ason_copy(b);

	return ret;
}

/**
 * Disjoin two ASON values.
 **/
ason_t *
ason_union(ason_t *a, ason_t *b)
{
	return ason_operate(a, b, ASON_UNION);
}

/**
 * Intersect two ASON values.
 **/
ason_t *
ason_intersect(ason_t *a, ason_t *b)
{
	return ason_operate(a, b, ASON_INTERSECT);
}

/**
 * Query ASON value a by b.
 **/
ason_t *
ason_query(ason_t *a, ason_t *b)
{
	return ason_operate(a, b, ASON_QUERY);
}

/**
 * Coquery ASON values a and b.
 **/
ason_t *
ason_coquery(ason_t *a, ason_t *b)
{
	return ason_operate(a, b, ASON_COQUERY);
}

/**
 * Appent ASON value b to a.
 **/
ason_t *
ason_append(ason_t *a, ason_t *b)
{
	return ason_operate(a, b, ASON_APPEND);
}

/* Predeclaration */
static int ason_do_check_equality(ason_t *a, ason_t *b, int null_eq);

/**
 * See if a union is equal to another value.
 **/
static int
ason_check_union_equals(ason_t *un, ason_t *other)
{
	size_t i;

	for (i = 0; i < un->count; i++)
		if (ason_do_check_equality(un->items[i], other, 0))
			return 1;

	return 0;
}

/**
 * Check if two lists are equal.
 **/
static int
ason_check_lists_equal(ason_t *a, ason_t *b)
{
	size_t i;

	for (i = 0; i < a->count && i < b->count; i++)
		if (! ason_check_equality(a->items[i], b->items[i]))
			return 0;

	for (; i < a->count; i++)
		if (! IS_NULL(a))
			return 0;

	for (; i < b->count; i++)
		if (! IS_NULL(b))
			return 0;

	return 1;
}

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
 * Check if two ASON objects are equal.
 **/
static int
ason_check_objects_equal(ason_t *a, ason_t *b)
{
	size_t a_i = 0;
	size_t b_i = 0;
	int cmp;

	ason_object_sort_kvs(a);
	ason_object_sort_kvs(b);

	while (a_i < a->count && b_i < b->count) {
		cmp = strcmp(a->kvs[a_i].key, b->kvs[b_i].key);

		if (! cmp && ! ason_check_equality(a->kvs[a_i].value,
						   b->kvs[b_i].value)) {
			return 0;
		} else if (cmp < 0 && b->type != ASON_UOBJECT) {
			return 0;
		} else if (cmp > 0 && a->type != ASON_UOBJECT) {
			return 0;
		}

		if (cmp <= 0)
			a_i++;

		if (cmp >= 0)
			b_i++;
	}

	for (; a_i < a->count; a_i++) {
		if (b->type == ASON_UOBJECT)
			return 1;

		if (! IS_NULL(a->kvs[a_i].value))
			return 0;
	}

	for (; b_i < b->count; b_i++) {
		if (a->type == ASON_UOBJECT)
			return 1;

		if (! IS_NULL(b->kvs[b_i].value))
			return 0;
	}

	return 1;
}

/**
 * Distribute an operator through a union on the left-hand side.
 **/
static ason_t *
ason_distribute_left(ason_t *un, ason_t *operand, ason_type_t operator)
{
	ason_t *ret;
	size_t i;

	ret = ason_create(ASON_UNION, un->count, 0);

	for (i = 0; i < ret->count; i++)
		ret->items[i] = ason_operate(un->items[i], operand,
					       operator);

	return ret;
}

/**
 * Distribute an operator through a union on the right-hand side.
 **/
static ason_t *
ason_distribute_right(ason_t *operand, ason_t *un, ason_type_t operator)
{
	ason_t *ret;
	size_t i;

	ret = ason_create(ASON_UNION, un->count, 0);

	for (i = 0; i < ret->count; i++)
		ret->items[i] = ason_operate(operand, un->items[i],
					       operator);

	return ret;
}

/**
 * Reduce an intersect of two objects.
 **/
static ason_t *
ason_reduce_object_intersect(ason_t *a, ason_t *b)
{
	size_t a_i = 0;
	size_t b_i = 0;
	size_t ret_i = 0;
	int cmp;
	ason_t *ret;

	ason_object_sort_kvs(a);
	ason_object_sort_kvs(b);

	if (a->type == ASON_UOBJECT && b->type == ASON_UOBJECT)
		ret = ason_create(ASON_UOBJECT, a->count + b->count, 1);
	else
		ret = ason_create(ASON_OBJECT, a->count + b->count, 1);

	while (a_i < a->count && b_i < b->count) {
		cmp = strcmp(a->kvs[a_i].key, b->kvs[b_i].key);

		if (! cmp) {
			ret->kvs[ret_i].key = xstrdup(a->kvs[a_i].key);
			ret->kvs[ret_i].value = ason_intersect(
			       a->kvs[a_i].value, b->kvs[b_i].value);
			ret_i++;
		} else if (cmp < 0 && b->type == ASON_UOBJECT) {
			ret->kvs[ret_i].key = xstrdup(a->kvs[a_i].key);
			ret->kvs[ret_i].value = ason_copy(a->kvs[a_i].value);
			ret_i++;
		} else if (cmp > 0 && a->type == ASON_UOBJECT) {
			ret->kvs[ret_i].key = xstrdup(b->kvs[b_i].key);
			ret->kvs[ret_i].value = ason_copy(b->kvs[b_i].value);
			ret_i++;
		}

		if (cmp <= 0)
			a_i++;

		if (cmp >= 0)
			b_i++;
	}

	for (; a_i < a->count; a_i++) {
		if (b->type != ASON_UOBJECT)
			continue;

		ret->kvs[ret_i].key = xstrdup(a->kvs[a_i].key);
		ret->kvs[ret_i].value = ason_copy(a->kvs[a_i].value);
		ret_i++;
	}

	for (; b_i < b->count; b_i++) {
		if (a->type != ASON_UOBJECT)
			continue;

		ret->kvs[ret_i].key = xstrdup(b->kvs[b_i].key);
		ret->kvs[ret_i].value = ason_copy(b->kvs[b_i].value);
		ret_i++;
	}

	return ret;
}

/**
 * Reduce an intersect of two lists.
 **/
static ason_t *
ason_reduce_list_intersect(ason_t *a, ason_t *b)
{
	size_t i = 0;
	ason_t *ret;
	size_t count = a->count;
	ason_t *a_sub;
	ason_t *b_sub;

	if (b->count > count)
		count = b->count;

	ret = ason_create(ASON_LIST, count, 0);

	for (; i < count; i++) {
		a_sub = b_sub = VALUE_NULL;

		if (i < a->count)
			a_sub = a->items[i];

		if (i < b->count)
			b_sub = b->items[i];

		ret->items[i] = ason_intersect(a_sub, b_sub);
	}

	return ret;
}

/**
 * Reduce an intersect of two non-union values.
 **/
static ason_t *
ason_reduce_intersect(ason_t *a, ason_t *b)
{
	if (IS_OBJECT(a) && IS_OBJECT(b))
		return ason_reduce_object_intersect(a, b);

	if (a->type == ASON_LIST && b->type == ASON_LIST)
		return ason_reduce_list_intersect(a, b);

	if (ason_check_equality(a, b))
		return ason_copy(a);

	return VALUE_NULL;
}

/**
 * Reduce a query of two non-union values.
 **/
static ason_t *
ason_reduce_query(ason_t *a, ason_t *b)
{
	ason_t *ret = ason_reduce_intersect(a, b);

	if (ason_check_represented_in(ret, b))
		return ret;

	ason_destroy(ret);

	return VALUE_NULL;
}

/**
 * Reduce a coquery of two non-union values.
 **/
static ason_t *
ason_reduce_coquery(ason_t *a, ason_t *b)
{
	ason_t *ret = ason_reduce_query(a, b);

	if (ason_check_represented_in(ret, a))
		return ret;

	ason_destroy(ret);

	return VALUE_NULL;
}

/**
 * Reduce an append operator between two lists.
 **/
static ason_t *
ason_reduce_list_append(ason_t *a, ason_t *b)
{
	ason_t *ret;
	size_t i;

	ret = ason_create(ASON_LIST, a->count + b->count, 0);

	memcpy(ret->items, a->items, a->count * sizeof(ason_t));
	memcpy(ret->items + a->count, b->items,
	       b->count * sizeof(ason_t));

	for (i = 0; i < ret->count; i++)
		ret->items[i] = ason_copy(ret->items[i]);

	return ret;
}

/**
 * Reduce an append of two objects.
 **/
static ason_t *
ason_reduce_object_append(ason_t *a, ason_t *b)
{
	size_t a_i = 0;
	size_t b_i = 0;
	size_t ret_i = 0;
	int cmp;
	ason_t *ret;

	ason_object_sort_kvs(a);
	ason_object_sort_kvs(b);


	if (a->type == ASON_UOBJECT || b->type == ASON_UOBJECT)
		ret = ason_create(ASON_UOBJECT, a->count + b->count, 0);
	else
		ret = ason_create(ASON_OBJECT, a->count + b->count, 0);

	while (a_i < a->count && b_i < b->count) {
		cmp = strcmp(a->kvs[a_i].key, b->kvs[b_i].key);

		if (! cmp) {
			ret->kvs[ret_i].key = xstrdup(a->kvs[a_i].key);
			ret->kvs[ret_i].value = ason_coquery(
			       a->kvs[a_i].value, b->kvs[b_i].value);
			ret_i++;
		} else if (cmp < 0) {
			ret->kvs[ret_i].key = xstrdup(ret->kvs[ret_i].key);
			ret->kvs[ret_i].value = ason_copy(a->kvs[a_i].value);
			ret_i++;
		} else if (cmp > 0) {
			ret->kvs[ret_i].key = xstrdup(ret->kvs[ret_i].key);
			ret->kvs[ret_i].value = ason_copy(b->kvs[b_i].value);
			ret_i++;
		}

		if (cmp <= 0)
			a_i++;

		if (cmp >= 0)
			b_i++;
	}

	for (; a_i < a->count; a_i++) {
		ret->kvs[ret_i].key = xstrdup(ret->kvs[ret_i].key);
		ret->kvs[ret_i].value = ason_copy(a->kvs[a_i].value);
		ret_i++;
	}

	for (; b_i < b->count; b_i++) {
		ret->kvs[ret_i].key = xstrdup(ret->kvs[ret_i].key);
		ret->kvs[ret_i].value = ason_copy(b->kvs[b_i].value);
		ret_i++;
	}

	ret->count = ret_i;

	return ret;
}

/* Predeclaration */
static ason_t *ason_simplify_transform(ason_t *in);

/**
 * Reduce an append of two non-union values.
 **/
static ason_t *
ason_reduce_append(ason_t *a, ason_t *b)
{
	a = ason_simplify_transform(a);
	b = ason_simplify_transform(b);

	if (IS_OBJECT(a) && IS_OBJECT(b))
		return ason_reduce_object_append(a, b);

	if (a->type == ASON_LIST && b->type == ASON_LIST)
		return ason_reduce_list_append(a, b);

	ason_destroy(a);
	ason_destroy(b);

	return VALUE_NULL;
}

/**
 * Reduce an ASON value so it is not expressed, at the top level, as a query,
 * coquery, intersect, or append.
 **/
static ason_t *
ason_simplify_transform(ason_t *in)
{
	ason_t *a;
	ason_t *b;

	switch (in->type) {
	case ASON_INTERSECT:
	case ASON_QUERY:
	case ASON_COQUERY:
	case ASON_APPEND:
		break;
	default:
		return ason_copy(in);
	};

	a = ason_simplify_transform(in->items[0]);
	b = ason_simplify_transform(in->items[1]);

	ason_destroy(in->items[0]);
	ason_destroy(in->items[1]);

	in->items[0] = a;
	in->items[1] = b;

	if (in->items[0]->type == ASON_UNION)
		return ason_distribute_left(in->items[0], in->items[1],
					    in->type);

	if (in->items[1]->type == ASON_UNION)
		return ason_distribute_right(in->items[0], in->items[1],
					     in->type);

	switch (in->type) {
	case ASON_INTERSECT:
		return ason_reduce_intersect(in->items[0], in->items[1]);
	case ASON_QUERY:
		return ason_reduce_query(in->items[0], in->items[1]);
	case ASON_COQUERY:
		return ason_reduce_coquery(in->items[0], in->items[1]);
	case ASON_APPEND:
		return ason_reduce_append(in->items[0], in->items[1]);
	default:
		errx(1, "Unreachable statement at %s:%d", __FILE__, __LINE__);
	};
}

/**
 * Check equality of two ASON values. If null_eq is 0, NULL != STRONG_NULL
 **/
int
ason_do_check_equality(ason_t *a, ason_t *b, int null_eq)
{
	int ret;

	a = ason_simplify_transform(a);
	b = ason_simplify_transform(b);

	if (a->type == ASON_UNIVERSE || b->type == ASON_UNIVERSE)
		ret = 1;
	else if (a->type == ASON_WILD && ! IS_NULL(b))
		ret = 1;
	else if (b->type == ASON_WILD && ! IS_NULL(a))
		ret = 1;
	else if (a->type == ASON_WILD || b->type == ASON_WILD)
		ret = 0;
	else if (a->type == ASON_UNION)
		ret = ason_check_union_equals(a, b);
	else if (b->type == ASON_UNION)
		ret = ason_check_union_equals(b, a);
	else if (IS_OBJECT(a) && IS_OBJECT(b))
		ret = ason_check_objects_equal(a, b);
	else if (IS_NULL(a) && b->type == ASON_STRONG_NULL)
		ret = 1;
	else if (IS_NULL(b) && a->type == ASON_STRONG_NULL)
		ret = 1;
	else if (a->type != b->type)
		ret = 0;
	else if (IS_NULL(a))
		ret = null_eq;
	else if (IS_NULL(b))
		ret = ason_check_lists_equal(a, b);
	else
		ret = (a->n == b->n);

	ason_destroy(a);
	ason_destroy(b);

	return ret;
}

/**
 * Check equality of two ASON values.
 **/
int
ason_check_equality(ason_t *a, ason_t *b)
{
	return ason_do_check_equality(a, b, 1);
}

/* Predeclaration */
static ason_t *ason_flatten(ason_t *value);

/**
 * Flatten an ASON list.
 **/
static ason_t *
ason_flatten_list(ason_t *value)
{
	size_t i;
	size_t j;
	size_t k;
	ason_t *un;
	ason_t *ret;
	ason_t *tmp;

	for (i = 0; i < value->count; i++) {
		tmp = value->items[i];
		value->items[i] = ason_flatten(value->items[i]);
		ason_destroy(tmp);
	}

	for (i = 0; i < value->count; i++)
		if (value->items[i]->type == ASON_UNION)
			break;

	if (i == value->count)
		return ason_copy(value);

	un = value->items[i];

	ret = ason_create(ASON_UNION, un->count, 0);

	for (j = 0; j < ret->count; j++) {
		ret->items[j] = ason_create(ASON_LIST, value->count, 0);

		for (k = 0; k < ret->items[j]->count; k++)
			if (k != i)
				ret->items[j]->items[k] =
					ason_copy(value->items[k]);

		ret->items[j]->items[i] = ason_copy(un->items[j]);
	}

	tmp = ason_flatten(ret);
	ason_destroy(ret);
	return tmp;
}

/**
 * Flatten an ASON object.
 **/
static ason_t *
ason_flatten_object(ason_t *value)
{
	size_t i;
	size_t j;
	size_t k;
	ason_t *un;
	ason_t *ret;
	ason_t *tmp;

	for (i = 0; i < value->count; i++) {
		tmp = value->kvs[i].value;
		value->kvs[i].value = ason_flatten(value->items[i]);
		ason_destroy(tmp);
	}

	for (i = 0; i < value->count; i++)
		if (value->kvs[i].value->type == ASON_UNION)
			break;

	if (i == value->count)
		return ason_copy(value);

	un = value->kvs[i].value;

	ret = ason_create(ASON_UNION, un->count, 0);

	for (j = 0; j < ret->count; j++) {
		ret->items[j] = ason_create(value->type, value->count, 1);

		for (k = 0; k < ret->items[j]->count; k++) {
			ret->items[j]->kvs[k].key =
				xstrdup(value->kvs[k].key);

			if (k != i)
				ret->items[j]->kvs[k].value =
					ason_copy(value->kvs[k].value);
		}

		ret->items[j]->kvs[i].value = ason_copy(un->items[j]);
	}

	tmp = ason_flatten(ret);
	ason_destroy(ret);
	return tmp;
}

/**
 * Ensure an ASON object has no indeterminate values.
 **/
static ason_t *
ason_flatten(ason_t *value)
{
	size_t i;
	size_t j;
	size_t k;
	size_t count;
	ason_t *ret;
	ason_t *tmp;

	value = ason_simplify_transform(value);

	switch (value->type) {
	case ASON_NUMERIC:
	case ASON_NULL:
	case ASON_STRONG_NULL:
	case ASON_WILD:
	case ASON_UNIVERSE:
		return value;
	case ASON_LIST:
		ret = ason_flatten_list(value);
		ason_destroy(value);
		return ret;
	case ASON_OBJECT:
		ret = ason_flatten_object(value);
		ason_destroy(value);
		return ret;
	case ASON_UNION:
		break;
	default:
		errx(1, "Unreachable statement at %s:%d", __FILE__, __LINE__);
	};

	for (i = 0; i < value->count; i++) {
		tmp = value->items[i];
		value->items[i] = ason_flatten(value->items[i]);
		ason_destroy(tmp);
	}

	count = 0;
	for (i = 0; i < value->count; i++) {
		if (value->type == ASON_UNION)
			count += value->count;
		else
			count += 1;
	}

	if (count == value->count)
		return value;

	ret = ason_create(ASON_UNION, count, 0);

	k = 0;
	for (i = 0; i < value->count; i++) {
		if (value->items[i]->type != ASON_UNION) {
			ret->items[k++] = ason_copy(value->items[i]);
		} else if (value->items[i]->type != ASON_NULL) {
			for (j = 0; j < value->items[i]->count; j++)
				ret->items[k++] =
					ason_copy(value->items[i]->items[j]);
		}
	}

	ason_destroy(value);

	return ret;
}

/**
 * Check whether ASON value a is represented in b.
 **/
int
ason_check_represented_in(ason_t *a, ason_t *b)
{
	size_t i;
	int ret;

	a = ason_flatten(a);
	b = ason_simplify_transform(b);

	if (a->type == ASON_UNION) {
		ret = 1;

		for (i = 0; i < a->count; i++)
			if (! ason_check_represented_in(a->items[i], b))
				ret = 0;
	} else if (b->type == ASON_UNION) {
		ret = 0;

		for (i = 0; i < b->count; i++)
			if (ason_check_represented_in(a, b->items[i]))
				ret = 1;
	} else if (b->type == ASON_UNIVERSE) {
		ret = 1;
	} else if (b->type == ASON_WILD) {
		ret = ! IS_NULL(a);
	} else {
		ret = ason_check_equality(a, b);
	}

	ason_destroy(a);
	ason_destroy(b);

	return ret;
}

/**
 * Check whether a and b are corepresentative.
 **/
int
ason_check_corepresented(ason_t *a, ason_t *b)
{
	/* FIXME: This can be made quicker */
	if (! ason_check_represented_in(a, b))
		return 0;
	if (! ason_check_represented_in(b, a))
		return 0;
	return 1;
}

/**
 * Get the type of an ASON value.
 **/
ason_type_t
ason_type(ason_t *value)
{
	return value->type;
}
