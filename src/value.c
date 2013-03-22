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

static inline void *xmalloc(size_t sz)
{
	void *ret = malloc(sz);

	if (! ret)
		errx(1, "Malloc failed");

	return ret;
}

static inline void *xcalloc(size_t memb, size_t sz)
{
	void *ret = calloc(memb, sz);

	if (! ret)
		errx(1, "Malloc failed");

	return ret;
}

static inline void *xstrdup(const char *str)
{
	void *ret = strdup(str);

	if (! ret)
		errx(1, "Malloc failed");

	return ret;
}

/**
 * Test if an ASON value is an object.
 **/
#define IS_OBJECT(_x) (_x.v->type == ASON_OBJECT || _x.v->type == ASON_UOBJECT)
#define IS_NULL(_x) (_x.v->type == ASON_NULL || _x.v->type == ASON_STRONG_NULL)

/**
 * A Key-value pair.
 **/
struct kv_pair {
	const char *key;
	ason_t value;
};

/**
 * Data making up a value.
 **/
struct ason {
	ason_type_t type;
	union {
		int64_t n;
		uint64_t u;
		ason_t *items;
		struct kv_pair *kvs;
	};

	size_t count;
};

/**
 * Handy value constants.
 **/
static struct ason VALUE_NULL_DATA = {
	.type = ASON_NULL,
	.items = NULL,
	.count = 0,
};
const ason_t VALUE_NULL = { .v = &VALUE_NULL_DATA, };

static struct ason VALUE_STRONG_NULL_DATA = {
	.type = ASON_STRONG_NULL,
	.items = NULL,
	.count = 0,
};
const ason_t VALUE_STRONG_NULL = { .v = &VALUE_STRONG_NULL_DATA, };

static struct ason VALUE_UNIVERSE_DATA = {
	.type = ASON_UNIVERSE,
	.items = NULL,
	.count = 0,
};
const ason_t VALUE_UNIVERSE = { .v = &VALUE_UNIVERSE_DATA, };

static struct ason VALUE_WILD_DATA = {
	.type = ASON_WILD,
	.items = NULL,
	.count = 0,
};
const ason_t VALUE_WILD = { .v = &VALUE_WILD_DATA, };

static struct ason VALUE_OBJ_ANY_DATA = {
	.type = ASON_UOBJECT,
	.items = NULL,
	.count = 0,
};
const ason_t VALUE_OBJ_ANY = { .v = &VALUE_OBJ_ANY_DATA, };

/**
 * Create an ASON numeric value.
 **/
ason_t
ason_create_number(int number)
{
	ason_t ret;

	ret.v = xmalloc(sizeof(struct ason));

	ret.v->type = ASON_NUMERIC;
	ret.v->n = number;
	ret.v->count = 0;

	return ret;
}

/**
 * Create an ASON list value.
 **/
ason_t
ason_create_list(ason_t content)
{
	ason_t ret;

	ret.v = xmalloc(sizeof(struct ason));

	ret.v->type = ASON_LIST;
	if (! IS_NULL(content)) {
		ret.v->items = xcalloc(1, sizeof(ason_t));
		ret.v->items[0] = content;
		ret.v->count = 1;
	} else {
		ret.v->items = NULL;
		ret.v->count = 0;
	}

	return ret;
}

/**
 * Create an ASON value.
 **/
ason_t
ason_create_object(const char *key, ason_t value) 
{
	ason_t ret;

	ret.v = xmalloc(sizeof(struct ason));

	ret.v->type = ASON_OBJECT;
	if (! IS_NULL(value)) {
		ret.v->kvs = xcalloc(1, sizeof(struct kv_pair));
		ret.v->kvs[0].key = xstrdup(key);
		ret.v->kvs[0].value = value;
		ret.v->count = 1;
	} else {
		ret.v->kvs = NULL;
		ret.v->count = 0;
	}

	return ret;
}

/**
 * Apply an operator to two ASON values.
 **/
static ason_t
ason_operate(ason_t a, ason_t b, ason_type_t type)
{
	ason_t ret;

	ret.v = xmalloc(sizeof(struct ason));

	ret.v->type = type;
	ret.v->items = xcalloc(2, sizeof(ason_t));
	ret.v->items[0] = a;
	ret.v->items[1] = b;
	ret.v->count = 2;

	return ret;
}

/**
 * Disjoin two ASON values.
 **/
ason_t
ason_disjoin(ason_t a, ason_t b)
{
	return ason_operate(a, b, ASON_DISJOIN);
}

/**
 * Intersect two ASON values.
 **/
ason_t
ason_overlap(ason_t a, ason_t b)
{
	return ason_operate(a, b, ASON_OVERLAP);
}

/**
 * Query ASON value a by b.
 **/
ason_t
ason_query(ason_t a, ason_t b)
{
	return ason_operate(a, b, ASON_QUERY);
}

/**
 * Coquery ASON values a and b.
 **/
ason_t
ason_intersect(ason_t a, ason_t b)
{
	return ason_operate(a, b, ASON_INTERSECT);
}

/**
 * Appent ASON value b to a.
 **/
ason_t
ason_append(ason_t a, ason_t b)
{
	return ason_operate(a, b, ASON_APPEND);
}

/* Predeclaration */
static int ason_do_check_equality(ason_t a, ason_t b, int null_eq);

/**
 * See if a disjoin is equal to another value.
 **/
static int
ason_check_disjoin_equals(ason_t disjoin, ason_t other)
{
	size_t i;

	for (i = 0; i < disjoin.v->count; i++)
		if (ason_do_check_equality(disjoin.v->items[i], other, 0))
			return 1;

	return 0;
}

/**
 * Check if two lists are equal.
 **/
static int
ason_check_lists_equal(ason_t a, ason_t b)
{
	size_t i;

	for (i = 0; i < a.v->count && i < b.v->count; i++)
		if (! ason_check_equality(a.v->items[i], b.v->items[i]))
			return 0;

	for (; i < a.v->count; i++)
		if (! IS_NULL(a))
			return 0;

	for (; i < b.v->count; i++)
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
ason_object_sort_kvs(ason_t obj)
{
	struct kv_pair *kvs = obj.v->kvs;
	size_t count = obj.v->count;

	kv_pair_quicksort(kvs, count);
}

/**
 * Check if two ASON objects are equal.
 **/
static int
ason_check_objects_equal(ason_t a, ason_t b)
{
	size_t a_i = 0;
	size_t b_i = 0;
	int cmp;

	ason_object_sort_kvs(a);
	ason_object_sort_kvs(b);

	while (a_i < a.v->count && b_i < b.v->count) {
		cmp = strcmp(a.v->kvs[a_i].key, b.v->kvs[b_i].key);

		if (! cmp && ! ason_check_equality(a.v->kvs[a_i].value,
						   b.v->kvs[b_i].value)) {
			return 0;
		} else if (cmp < 0 && b.v->type != ASON_UOBJECT) {
			return 0;
		} else if (cmp > 0 && a.v->type != ASON_UOBJECT) {
			return 0;
		}

		if (cmp <= 0)
			a_i++;

		if (cmp >= 0)
			b_i++;
	}

	for (; a_i < a.v->count; a_i++) {
		if (b.v->type == ASON_UOBJECT)
			return 1;

		if (! IS_NULL(a.v->kvs[a_i].value))
			return 0;
	}

	for (; b_i < b.v->count; b_i++) {
		if (a.v->type == ASON_UOBJECT)
			return 1;

		if (! IS_NULL(b.v->kvs[b_i].value))
			return 0;
	}

	return 1;
}

/**
 * Distribute an operator through a disjoin on the left-hand side.
 **/
static ason_t
ason_distribute_left(ason_t disjoin, ason_t operand, ason_type_t operator)
{
	ason_t ret;
	size_t i;

	ret.v = xmalloc(sizeof(struct ason));

	ret.v->type = ASON_DISJOIN;
	ret.v->items = xcalloc(disjoin.v->count, sizeof(ason_t));
	ret.v->count = disjoin.v->count;

	for (i = 0; i < ret.v->count; i++)
		ret.v->items[i] = ason_operate(disjoin.v->items[i], operand,
					       operator);

	return ret;
}

/**
 * Distribute an operator through a disjoin on the right-hand side.
 **/
static ason_t
ason_distribute_right(ason_t operand, ason_t disjoin, ason_type_t operator)
{
	ason_t ret;
	size_t i;

	ret.v = xmalloc(sizeof(struct ason));

	ret.v->type = ASON_DISJOIN;
	ret.v->items = xcalloc(disjoin.v->count, sizeof(ason_t));
	ret.v->count = disjoin.v->count;

	for (i = 0; i < ret.v->count; i++)
		ret.v->items[i] = ason_operate(operand, disjoin.v->items[i],
					       operator);

	return ret;
}

/**
 * Reduce an overlap of two objects.
 **/
static ason_t
ason_reduce_object_overlap(ason_t a, ason_t b)
{
	size_t a_i = 0;
	size_t b_i = 0;
	size_t ret_i = 0;
	int cmp;
	ason_t ret;

	ason_object_sort_kvs(a);
	ason_object_sort_kvs(b);

	ret.v = xmalloc(sizeof(struct ason));

	if (a.v->type == ASON_UOBJECT && b.v->type == ASON_UOBJECT)
		ret.v->type = ASON_UOBJECT;
	else
		ret.v->type = ASON_OBJECT;

	ret.v->count = a.v->count + b.v->count;
	ret.v->kvs = xcalloc(ret.v->count, sizeof(struct kv_pair));

	while (a_i < a.v->count && b_i < b.v->count) {
		cmp = strcmp(a.v->kvs[a_i].key, b.v->kvs[b_i].key);

		if (! cmp) {
			ret.v->kvs[ret_i].key = xstrdup(a.v->kvs[a_i].key);
			ret.v->kvs[ret_i].value = ason_overlap(
			       a.v->kvs[a_i].value, b.v->kvs[b_i].value);
			ret_i++;
		} else if (cmp < 0 && b.v->type == ASON_UOBJECT) {
			ret.v->kvs[ret_i].key = xstrdup(a.v->kvs[a_i].key);
			ret.v->kvs[ret_i].value = a.v->kvs[a_i].value;
			ret_i++;
		} else if (cmp > 0 && a.v->type == ASON_UOBJECT) {
			ret.v->kvs[ret_i].key = xstrdup(b.v->kvs[b_i].key);
			ret.v->kvs[ret_i].value = b.v->kvs[b_i].value;
			ret_i++;
		}

		if (cmp <= 0)
			a_i++;

		if (cmp >= 0)
			b_i++;
	}

	for (; a_i < a.v->count; a_i++) {
		if (b.v->type != ASON_UOBJECT)
			continue;

		ret.v->kvs[ret_i].key = xstrdup(a.v->kvs[a_i].key);
		ret.v->kvs[ret_i].value = a.v->kvs[a_i].value;
		ret_i++;
	}

	for (; b_i < b.v->count; b_i++) {
		if (a.v->type != ASON_UOBJECT)
			continue;

		ret.v->kvs[ret_i].key = xstrdup(b.v->kvs[b_i].key);
		ret.v->kvs[ret_i].value = b.v->kvs[b_i].value;
		ret_i++;
	}

	return ret;
}

/**
 * Reduce an overlap of two lists.
 **/
static ason_t
ason_reduce_list_overlap(ason_t a, ason_t b)
{
	size_t i = 0;
	ason_t ret;
	size_t count = a.v->count;
	ason_t a_sub;
	ason_t b_sub;

	if (b.v->count > count)
		count = b.v->count;

	ret.v = xmalloc(sizeof(struct ason));

	ret.v->type = ASON_LIST;
	ret.v->items = xcalloc(count, sizeof(ason_t));
	ret.v->count = count;

	for (; i < count; i++) {
		a_sub = b_sub = VALUE_NULL;

		if (i < a.v->count)
			a_sub = a.v->items[i];

		if (i < b.v->count)
			b_sub = b.v->items[i];

		ret.v->items[i] = ason_overlap(a_sub, b_sub);
	}

	return ret;
}

/**
 * Reduce an overlap of two non-disjoin values.
 **/
static ason_t
ason_reduce_overlap(ason_t a, ason_t b)
{
	if (IS_OBJECT(a) && IS_OBJECT(b))
		return ason_reduce_object_overlap(a, b);

	if (a.v->type == ASON_LIST && b.v->type == ASON_LIST)
		return ason_reduce_list_overlap(a, b);

	if (ason_check_equality(a, b))
		return a;

	return VALUE_NULL;
}

/**
 * Reduce a query of two non-disjoin values.
 **/
static ason_t
ason_reduce_query(ason_t a, ason_t b)
{
	ason_t ret = ason_reduce_overlap(a, b);

	if (ason_check_represented_in(ret, b))
		return ret;

	return VALUE_NULL;
}

/**
 * Reduce a intersect of two non-disjoin values.
 **/
static ason_t
ason_reduce_intersect(ason_t a, ason_t b)
{
	ason_t ret = ason_reduce_query(a, b);

	if (ason_check_represented_in(ret, a))
		return ret;

	return VALUE_NULL;
}

/**
 * Reduce an append operator between two lists.
 **/
static ason_t
ason_reduce_list_append(ason_t a, ason_t b)
{
	ason_t ret;

	ret.v = xmalloc(sizeof(struct ason));

	ret.v->count = a.v->count + b.v->count;
	ret.v->type = ASON_LIST;
	ret.v->items = xcalloc(ret.v->count, sizeof(ason_t));

	memcpy(ret.v->items, a.v->items, a.v->count * sizeof(ason_t));
	memcpy(ret.v->items + a.v->count, b.v->items,
	       b.v->count * sizeof(ason_t));

	return ret;
}

/**
 * Reduce an append of two objects.
 **/
static ason_t
ason_reduce_object_append(ason_t a, ason_t b)
{
	size_t a_i = 0;
	size_t b_i = 0;
	size_t ret_i = 0;
	int cmp;
	ason_t ret;

	ason_object_sort_kvs(a);
	ason_object_sort_kvs(b);

	ret.v = xmalloc(sizeof(struct ason));

	if (a.v->type == ASON_UOBJECT || b.v->type == ASON_UOBJECT)
		ret.v->type = ASON_UOBJECT;
	else
		ret.v->type = ASON_OBJECT;

	ret.v->count = a.v->count + b.v->count;
	ret.v->kvs = xcalloc(ret.v->count, sizeof(struct kv_pair));

	while (a_i < a.v->count && b_i < b.v->count) {
		cmp = strcmp(a.v->kvs[a_i].key, b.v->kvs[b_i].key);

		if (! cmp) {
			ret.v->kvs[ret_i].key = xstrdup(a.v->kvs[a_i].key);
			ret.v->kvs[ret_i].value = ason_intersect(
			       a.v->kvs[a_i].value, b.v->kvs[b_i].value);
			ret_i++;
		} else if (cmp < 0) {
			ret.v->kvs[ret_i] = a.v->kvs[a_i];
			ret.v->kvs[ret_i].key = xstrdup(ret.v->kvs[ret_i].key);
			ret_i++;
		} else if (cmp > 0) {
			ret.v->kvs[ret_i] = b.v->kvs[b_i];
			ret.v->kvs[ret_i].key = xstrdup(ret.v->kvs[ret_i].key);
			ret_i++;
		}

		if (cmp <= 0)
			a_i++;

		if (cmp >= 0)
			b_i++;
	}

	for (; a_i < a.v->count; a_i++) {
		ret.v->kvs[ret_i] = a.v->kvs[a_i];
		ret.v->kvs[ret_i].key = xstrdup(ret.v->kvs[ret_i].key);
		ret_i++;
	}

	for (; b_i < b.v->count; b_i++) {
		ret.v->kvs[ret_i] = b.v->kvs[b_i];
		ret.v->kvs[ret_i].key = xstrdup(ret.v->kvs[ret_i].key);
		ret_i++;
	}

	return ret;
}

/* Predeclaration */
static ason_t ason_simplify_transform(ason_t in);

/**
 * Reduce an append of two non-disjoin values.
 **/
static ason_t
ason_reduce_append(ason_t a, ason_t b)
{
	ason_simplify_transform(a);
	ason_simplify_transform(b);

	if (IS_OBJECT(a) && IS_OBJECT(b))
		return ason_reduce_object_append(a, b);

	if (a.v->type == ASON_LIST && b.v->type == ASON_LIST)
		return ason_reduce_list_append(a, b);

	return VALUE_NULL;
}

/**
 * Reduce an ASON value so it is not expressed, at the top level, as a query,
 * intersect, overlap, or append.
 **/
static ason_t
ason_simplify_transform(ason_t in)
{
	switch (in.v->type) {
	case ASON_OVERLAP:
	case ASON_QUERY:
	case ASON_INTERSECT:
	case ASON_APPEND:
		break;
	default:
		return in;
	};

	in.v->items[0] = ason_simplify_transform(in.v->items[0]);
	in.v->items[1] = ason_simplify_transform(in.v->items[1]);

	if (in.v->items[0].v->type == ASON_DISJOIN)
		return ason_distribute_left(in.v->items[0], in.v->items[1],
					    in.v->type);

	if (in.v->items[1].v->type == ASON_DISJOIN)
		return ason_distribute_right(in.v->items[0], in.v->items[1],
					     in.v->type);

	switch (in.v->type) {
	case ASON_OVERLAP:
		return ason_reduce_overlap(in.v->items[0], in.v->items[1]);
	case ASON_QUERY:
		return ason_reduce_query(in.v->items[0], in.v->items[1]);
	case ASON_INTERSECT:
		return ason_reduce_intersect(in.v->items[0], in.v->items[1]);
	case ASON_APPEND:
		return ason_reduce_append(in.v->items[0], in.v->items[1]);
	default:
		errx(1, "Unreachable statement at %s:%d", __FILE__, __LINE__);
	};
}

/**
 * Check equality of two ASON values. If null_eq is 0, NULL != STRONG_NULL
 **/
int
ason_do_check_equality(ason_t a, ason_t b, int null_eq)
{
	a = ason_simplify_transform(a);
	b = ason_simplify_transform(b);

	if (a.v->type == ASON_UNIVERSE || b.v->type == ASON_UNIVERSE)
		return 1;

	if (a.v->type == ASON_WILD && ! IS_NULL(b))
		return 1;

	if (b.v->type == ASON_WILD && ! IS_NULL(a))
		return 1;

	if (a.v->type == ASON_WILD || b.v->type == ASON_WILD)
		return 0;

	if (a.v->type == ASON_DISJOIN)
		return ason_check_disjoin_equals(a, b);
	
	if (b.v->type == ASON_DISJOIN)
		return ason_check_disjoin_equals(b, a);

	if (IS_OBJECT(a) && IS_OBJECT(b))
		return ason_check_objects_equal(a, b);

	if (IS_NULL(a) && b.v->type == ASON_STRONG_NULL)
		return 1;

	if (IS_NULL(b) && a.v->type == ASON_STRONG_NULL)
		return 1;

	if (a.v->type != b.v->type)
		return 0;

	if (IS_NULL(a))
		return null_eq;

	if (IS_NULL(b))
		return ason_check_lists_equal(a, b);

	return a.v->n == b.v->n;
}

/**
 * Check equality of two ASON values.
 **/
int
ason_check_equality(ason_t a, ason_t b)
{
	return ason_do_check_equality(a, b, 1);
}

/* Predeclaration */
static ason_t ason_flatten(ason_t value);

/**
 * Flatten an ASON list.
 **/
static ason_t
ason_flatten_list(ason_t value)
{
	size_t i;
	size_t j;
	ason_t disjoin;
	ason_t ret;

	for (i = 0; i < value.v->count; i++)
		value.v->items[i] = ason_flatten(value.v->items[i]);

	for (i = 0; i < value.v->count; i++)
		if (value.v->items[i].v->type == ASON_DISJOIN)
			break;

	if (i == value.v->count)
		return value;

	disjoin = value.v->items[i];

	ret.v = xmalloc(sizeof(struct ason));

	ret.v->type = ASON_DISJOIN;
	ret.v->count = disjoin.v->count;
	ret.v->items = xcalloc(ret.v->count, sizeof(ason_t));

	for (j = 0; j < ret.v->count; j++) {
		ret.v->items[j].v = xmalloc(sizeof(struct ason));

		ret.v->items[j].v->type = ASON_LIST;
		ret.v->items[j].v->count = value.v->count;
		ret.v->items[j].v->items = xcalloc(value.v->count,
						   sizeof(ason_t));

		memcpy(ret.v->items[j].v->items, value.v->items,
		       value.v->count * sizeof(ason_t));

		ret.v->items[j].v->items[i] = disjoin.v->items[j];
	}

	return ason_flatten(ret);
}

/**
 * Flatten an ASON object.
 **/
static ason_t
ason_flatten_object(ason_t value)
{
	size_t i;
	size_t j;
	size_t k;
	ason_t disjoin;
	ason_t ret;

	for (i = 0; i < value.v->count; i++)
		value.v->kvs[i].value = ason_flatten(value.v->items[i]);

	for (i = 0; i < value.v->count; i++)
		if (value.v->kvs[i].value.v->type == ASON_DISJOIN)
			break;

	if (i == value.v->count)
		return value;

	disjoin = value.v->kvs[i].value;

	ret.v = xmalloc(sizeof(struct ason));

	ret.v->type = ASON_DISJOIN;
	ret.v->count = disjoin.v->count;
	ret.v->items = xcalloc(ret.v->count, sizeof(ason_t));

	for (j = 0; j < ret.v->count; j++) {
		ret.v->items[j].v = xmalloc(sizeof(struct ason));

		ret.v->items[j].v->type = value.v->type;
		ret.v->items[j].v->count = value.v->count;
		ret.v->items[j].v->kvs = xcalloc(value.v->count,
						   sizeof(struct kv_pair));

		memcpy(ret.v->items[j].v->kvs, value.v->kvs,
		       value.v->count * sizeof(struct kv_pair));

		for (k = 0; k < ret.v->items[j].v->count; k++)
			ret.v->items[j].v->kvs[k].key =
				xstrdup(ret.v->items[j].v->kvs[k].key);

		ret.v->items[j].v->kvs[i].value = disjoin.v->items[j];
	}

	return ason_flatten(ret);
}

/**
 * Ensure an ASON object has no indeterminate values.
 **/
static ason_t
ason_flatten(ason_t value)
{
	size_t i;
	size_t j;
	size_t k;
	size_t count;
	ason_t ret;

	value = ason_simplify_transform(value);

	switch (value.v->type) {
	case ASON_NUMERIC:
	case ASON_NULL:
	case ASON_STRONG_NULL:
	case ASON_WILD:
	case ASON_UNIVERSE:
		return value;
	case ASON_LIST:
		return ason_flatten_list(value);
	case ASON_OBJECT:
		return ason_flatten_object(value);
	case ASON_DISJOIN:
		break;
	default:
		errx(1, "Unreachable statement at %s:%d", __FILE__, __LINE__);
	};

	for (i = 0; i < value.v->count; i++) {
		value.v->items[i] = ason_flatten(value.v->items[i]);
	}

	count = 0;
	for (i = 0; i < value.v->count; i++) {
		if (value.v->type == ASON_DISJOIN)
			count += value.v->count;
		else
			count += 1;
	}

	if (count == value.v->count)
		return value;

	ret.v = xmalloc(sizeof(struct ason));

	ret.v->type = ASON_DISJOIN;
	ret.v->items = xcalloc(count, sizeof(ason_t));
	ret.v->count = count;

	k = 0;
	for (i = 0; i < value.v->count; i++) {
		if (value.v->items[i].v->type != ASON_DISJOIN) {
			ret.v->items[k++] = value.v->items[i];
		} else if (value.v->items[i].v->type != ASON_NULL) {
			for (j = 0; j < value.v->items[i].v->count; j++)
				ret.v->items[k++] =
					value.v->items[i].v->items[j];
		}
	}

	return ret;
}

/**
 * Check whether ASON value a is represented in b.
 **/
int
ason_check_represented_in(ason_t a, ason_t b)
{
	size_t i;

	a = ason_flatten(a);
	b = ason_simplify_transform(b);

	if (a.v->type == ASON_DISJOIN) {
		for (i = 0; i < a.v->count; i++)
			if (! ason_check_represented_in(a.v->items[i], b))
				return 0;

		return 1;
	}

	if (b.v->type == ASON_DISJOIN) {
		for (i = 0; i < b.v->count; i++)
			if (ason_check_represented_in(a, b.v->items[i]))
				return 1;

		return 0;
	}

	if (b.v->type == ASON_UNIVERSE)
		return 1;

	if (b.v->type == ASON_WILD)
		return a.v->type != ASON_NULL;

	return ason_check_equality(a, b);
}

/**
 * Check whether a and b are corepresentative.
 **/
int
ason_check_corepresented(ason_t a, ason_t b)
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
ason_type(ason_t value)
{
	return value.v->type;
}
