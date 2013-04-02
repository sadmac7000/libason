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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <err.h>

#include <ason/value.h>

#include "value.h"
#include "util.h"
#include "stringfunc.h"

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
static struct ason VALUE_EMPTY_DATA = {
	.type = ASON_EMPTY,
	.items = NULL,
	.count = 0,
};
ason_t * const VALUE_EMPTY = &VALUE_EMPTY_DATA;

static struct ason VALUE_NULL_DATA = {
	.type = ASON_NULL,
	.items = NULL,
	.count = 0,
};
ason_t * const VALUE_NULL = &VALUE_NULL_DATA;

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
	*a = *b = VALUE_NULL;

	if (iter->a->type == ASON_UOBJECT)
		*a = VALUE_UNIVERSE;

	if (iter->b->type == ASON_UOBJECT)
		*b = VALUE_UNIVERSE;

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

	if (! ret->count)
		return ret;

	if (type == ASON_OBJECT || type == ASON_UOBJECT)
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
	if (a == VALUE_EMPTY		||
	    a == VALUE_NULL	||
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

	if (a == VALUE_EMPTY		||
	    a == VALUE_NULL	||
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
	ason_t *ret = ason_create(ASON_NUMERIC, 0);

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
		return ason_create(ASON_LIST, 0);

	ret = ason_create(ASON_LIST, 1);
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
		return ason_create(ASON_OBJECT, 0);

	if (value->type == ASON_EMPTY)
		return VALUE_EMPTY;

	ret = ason_create(ASON_OBJECT, 1);

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
 * Appent ASON value b to a.
 **/
ason_t *
ason_append(ason_t *a, ason_t *b)
{
	return ason_operate(a, b, ASON_APPEND);
}

/**
 * Distribute an operator through a union on the left-hand side.
 **/
static ason_t *
ason_distribute_left(ason_t *un, ason_t *operand, ason_type_t operator)
{
	ason_t *ret;
	size_t i;

	ret = ason_create(ASON_UNION, un->count);

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

	ret = ason_create(ASON_UNION, un->count);

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
	size_t i;
	ason_t *ret;
	const char *key;
	struct ason_coiterator iter;

	ason_coiterator_init(&iter, a, b);

	if (a->type == ASON_UOBJECT && b->type == ASON_UOBJECT)
		ret = ason_create(ASON_UOBJECT, a->count + b->count);
	else
		ret = ason_create(ASON_OBJECT, a->count + b->count);

	ret->count = 0;

	for (i = 0; (key = ason_coiterator_next(&iter, &a, &b)); i++) {
		ret->kvs[i].key = xstrdup(key);
		ret->kvs[i].value = ason_intersect(a, b);
		ret->count++;
	}

	ason_coiterator_release(&iter);
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

	ret = ason_create(ASON_LIST, count);

	for (; i < count; i++) {
		a_sub = b_sub = VALUE_EMPTY;

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

	if (ason_check_equal(a, b))
		return ason_copy(a);

	return VALUE_EMPTY;
}

/**
 * Reduce an append operator between two lists.
 **/
static ason_t *
ason_reduce_list_append(ason_t *a, ason_t *b)
{
	ason_t *ret;
	size_t i;

	ret = ason_create(ASON_LIST, a->count + b->count);

	memcpy(ret->items, a->items, a->count * sizeof(ason_t *));
	memcpy(ret->items + a->count, b->items,
	       b->count * sizeof(ason_t *));

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
	size_t i;
	const char *key;
	ason_t *value;
	ason_t *ret;
	struct ason_coiterator iter;

	ason_coiterator_init(&iter, a, b);

	if (a->type == ASON_UOBJECT || b->type == ASON_UOBJECT)
		ret = ason_create(ASON_UOBJECT, a->count + b->count);
	else
		ret = ason_create(ASON_OBJECT, a->count + b->count);

	ret->count = 0;

	for (i = 0; (key = ason_coiterator_next(&iter, &a, &b)); i++) {
		if (a->type == ASON_NULL)
			value = ason_copy(b);
		else if (b->type == ASON_NULL)
			value = ason_copy(a);
		else
			value = ason_intersect(a, b);

		ret->kvs[i].key = xstrdup(key);
		ret->kvs[i].value = value;
		ret->count++;
	}

	ason_coiterator_release(&iter);
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
	ason_t *ret = VALUE_EMPTY;

	a = ason_simplify_transform(a);
	b = ason_simplify_transform(b);

	if (IS_OBJECT(a) && IS_OBJECT(b))
		ret = ason_reduce_object_append(a, b);

	if (a->type == ASON_LIST && b->type == ASON_LIST)
		ret = ason_reduce_list_append(a, b);

	ason_destroy(a);
	ason_destroy(b);

	return ret;
}

/**
 * Reduce an ASON value so it is not expressed, at the top level, as an
 * intersect, or append.
 **/
static ason_t *
ason_simplify_transform(ason_t *in)
{
	ason_t *a;
	ason_t *b;
	size_t i;

	switch (in->type) {
	case ASON_INTERSECT:
	case ASON_APPEND:
		break;
	case ASON_OBJECT:
	case ASON_UOBJECT:
		for (i = 0; i < in->count; i++)
			if (ason_check_equal(in->kvs[i].value, VALUE_EMPTY))
				return VALUE_EMPTY;
		return ason_copy(in);
	case ASON_LIST:
		for (i = 0; i < in->count; i++)
			if (ason_check_equal(in->items[i], VALUE_EMPTY))
				return VALUE_EMPTY;
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
	case ASON_APPEND:
		return ason_reduce_append(in->items[0], in->items[1]);
	default:
		errx(1, "Unreachable statement at %s:%d", __FILE__, __LINE__);
	};
}

/**
 * Check intersectionality of two ASON values.
 **/
int
ason_check_intersects(ason_t *a, ason_t *b)
{
	ason_t *inter = ason_intersect(a, b);
	int ret = !ason_check_equal(inter, VALUE_EMPTY);

	ason_destroy(inter);
	return ret;
}

/* Predeclaration */
static ason_t *ason_flatten(ason_t *in);

/**
 * Destroy a value and return a flattened version.
 **/
static inline ason_t *
ason_flatten_d(ason_t *in)
{
	ason_t *ret;
	ret = ason_flatten(in);
	ason_destroy(in);
	return ret;
}

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

	for (i = 0; i < value->count; i++)
		value->items[i] = ason_flatten_d(value->items[i]);

	for (i = 0; i < value->count; i++)
		if (value->items[i]->type == ASON_UNION)
			break;

	if (i == value->count)
		return ason_copy(value);

	un = value->items[i];

	ret = ason_create(ASON_UNION, un->count);

	for (j = 0; j < ret->count; j++) {
		ret->items[j] = ason_create(ASON_LIST, value->count);

		for (k = 0; k < ret->items[j]->count; k++)
			if (k != i)
				ret->items[j]->items[k] =
					ason_copy(value->items[k]);

		ret->items[j]->items[i] = ason_copy(un->items[j]);
	}

	return ason_flatten_d(ret);
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

	for (i = 0; i < value->count; i++)
		value->kvs[i].value = ason_flatten_d(value->kvs[i].value);

	for (i = 0; i < value->count; i++)
		if (value->kvs[i].value->type == ASON_UNION)
			break;

	if (i == value->count)
		return ason_copy(value);

	un = value->kvs[i].value;

	ret = ason_create(ASON_UNION, un->count);

	for (j = 0; j < ret->count; j++) {
		ret->items[j] = ason_create(value->type, value->count);

		for (k = 0; k < ret->items[j]->count; k++) {
			ret->items[j]->kvs[k].key =
				xstrdup(value->kvs[k].key);

			if (k != i)
				ret->items[j]->kvs[k].value =
					ason_copy(value->kvs[k].value);
		}

		ret->items[j]->kvs[i].value = ason_copy(un->items[j]);
	}

	return ason_flatten_d(ret);
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

	value = ason_simplify_transform(value);

	switch (value->type) {
	case ASON_NUMERIC:
	case ASON_EMPTY:
	case ASON_NULL:
	case ASON_WILD:
	case ASON_UNIVERSE:
		return value;
	case ASON_LIST:
		ret = ason_flatten_list(value);
		ason_destroy(value);
		return ret;
	case ASON_OBJECT:
	case ASON_UOBJECT:
		ret = ason_flatten_object(value);
		ason_destroy(value);
		return ret;
	case ASON_UNION:
		break;
	default:
		errx(1, "Unreachable statement at %s:%d", __FILE__, __LINE__);
	};

	for (i = 0; i < value->count; i++)
		value->items[i] = ason_flatten_d(value->items[i]);

	count = 0;
	for (i = 0; i < value->count; i++) {
		if (value->items[i]->type == ASON_UNION)
			count += value->items[i]->count;
		else
			count += 1;
	}

	if (count == value->count)
		return value;

	ret = ason_create(ASON_UNION, count);

	k = 0;
	for (i = 0; i < value->count; i++) {
		if (value->items[i]->type == ASON_EMPTY)
			continue;

		if (value->items[i]->type != ASON_UNION) {
			ret->items[k++] = ason_copy(value->items[i]);
			continue;
		}

		for (j = 0; j < value->items[i]->count; j++)
			ret->items[k++] = ason_copy(value->items[i]->items[j]);
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
	int ret = 0;
	struct ason_coiterator iter;
	ason_t *c;
	ason_t *d;

	a = ason_flatten(a);
	b = ason_simplify_transform(b);

	if (a->type == ASON_EMPTY) {
		ret = 1; 
	} else if (b->type == ASON_UNIVERSE) {
		ret = 1;
	} else if (a->type == ASON_UNION) {
		ret = 1;

		for (i = 0; i < a->count; i++)
			if (! ason_check_represented_in(a->items[i], b))
				ret = 0;
	} else if (b->type == ASON_UNION) {
		for (i = 0; i < b->count; i++)
			if (ason_check_represented_in(a, b->items[i]))
				ret = 1;
	} else if (b->type == ASON_WILD) {
		ret = a->type != ASON_NULL;
	} else if (a->type == ASON_LIST && b->type == ASON_LIST) {
		ret = 1;

		for (i = a->count; ret && i < b->count; i++)
			ret = ason_check_represented_in(b->items[i],
							VALUE_EMPTY);

		for (i = 0; ret && i < a->count && i < b->count; i++)
			ret = ason_check_represented_in(a->items[i],
							b->items[i]);
	} else if (a->type == ASON_UOBJECT && b->type == ASON_OBJECT) {
		/* skip */
	} else if (IS_OBJECT(a) && IS_OBJECT(b)) {
		ret = 1;

		ason_coiterator_init(&iter, a, b);

		while (ret && ason_coiterator_next(&iter, &c, &d))
			ret = ason_check_represented_in(c, d);

		ason_coiterator_release(&iter);
	} else if (a->type == ASON_NUMERIC && b->type == ASON_NUMERIC) {
		ret = a->n == b->n;
	}

	ason_destroy(a);
	ason_destroy(b);

	return ret;
}

/**
 * Check whether a and b are equal.
 **/
int
ason_check_equal(ason_t *a, ason_t *b)
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
