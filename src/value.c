/**
 * This file is part of libjsonalg.
 *
 * libjsonalg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libjsonalg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libjsonalg. If not, see <http://www.gnu.org/licenses/>.
 **/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <err.h>

#include <jsonalg/value.h>

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
 * Test if a JSON value is an object.
 **/
#define IS_OBJECT(_x) (_x.v->type == JSON_OBJECT || _x.v->type == JSON_UOBJECT)
#define IS_NULL(_x) (_x.v->type == JSON_NULL || _x.v->type == JSON_STRONG_NULL)

/**
 * A Key-value pair.
 **/
struct kv_pair {
	const char *key;
	json_t value;
};

/**
 * Data making up a value.
 **/
struct value_data {
	json_type_t type;
	union {
		int64_t n;
		uint64_t u;
		json_t *items;
		struct kv_pair *kvs;
	};

	size_t count;
};

/**
 * Handy value constants.
 **/
static struct value_data VALUE_NULL_DATA = {
	.type = JSON_NULL,
	.items = NULL,
	.count = 0,
};
const json_t VALUE_NULL = { .v = &VALUE_NULL_DATA, };

static struct value_data VALUE_STRONG_NULL_DATA = {
	.type = JSON_STRONG_NULL,
	.items = NULL,
	.count = 0,
};
const json_t VALUE_STRONG_NULL = { .v = &VALUE_STRONG_NULL_DATA, };

static struct value_data VALUE_UNIVERSE_DATA = {
	.type = JSON_UNIVERSE,
	.items = NULL,
	.count = 0,
};
const json_t VALUE_UNIVERSE = { .v = &VALUE_UNIVERSE_DATA, };

static struct value_data VALUE_WILD_DATA = {
	.type = JSON_WILD,
	.items = NULL,
	.count = 0,
};
const json_t VALUE_WILD = { .v = &VALUE_WILD_DATA, };

static struct value_data VALUE_OBJ_ANY_DATA = {
	.type = JSON_UOBJECT,
	.items = NULL,
	.count = 0,
};
const json_t VALUE_OBJ_ANY = { .v = &VALUE_OBJ_ANY_DATA, };

/**
 * Create a JSON numeric value.
 **/
json_t
json_create_number(int number)
{
	json_t ret;

	ret.v = xmalloc(sizeof(struct value_data));

	ret.v->type = JSON_NUMERIC;
	ret.v->n = number;
	ret.v->count = 0;

	return ret;
}

/**
 * Create a JSON list value.
 **/
json_t
json_create_list(json_t content)
{
	json_t ret;

	ret.v = xmalloc(sizeof(struct value_data));

	ret.v->type = JSON_LIST;
	if (! IS_NULL(content)) {
		ret.v->items = xcalloc(1, sizeof(json_t));
		ret.v->items[0] = content;
		ret.v->count = 1;
	} else {
		ret.v->items = NULL;
		ret.v->count = 0;
	}

	return ret;
}

/**
 * Create a JSON value.
 **/
json_t
json_create_object(const char *key, json_t value) 
{
	json_t ret;

	ret.v = xmalloc(sizeof(struct value_data));

	ret.v->type = JSON_OBJECT;
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
 * Apply an operator to two JSON values.
 **/
static json_t
json_operate(json_t a, json_t b, json_type_t type)
{
	json_t ret;

	ret.v = xmalloc(sizeof(struct value_data));

	ret.v->type = type;
	ret.v->items = xcalloc(2, sizeof(json_t));
	ret.v->items[0] = a;
	ret.v->items[1] = b;
	ret.v->count = 2;

	return ret;
}

/**
 * Disjoin two JSON values.
 **/
json_t
json_disjoin(json_t a, json_t b)
{
	return json_operate(a, b, JSON_DISJOIN);
}

/**
 * Intersect two JSON values.
 **/
json_t
json_overlap(json_t a, json_t b)
{
	return json_operate(a, b, JSON_OVERLAP);
}

/**
 * Query JSON value a by b.
 **/
json_t
json_query(json_t a, json_t b)
{
	return json_operate(a, b, JSON_QUERY);
}

/**
 * Coquery JSON values a and b.
 **/
json_t
json_intersect(json_t a, json_t b)
{
	return json_operate(a, b, JSON_INTERSECT);
}

/**
 * Appent JSON value b to a.
 **/
json_t
json_append(json_t a, json_t b)
{
	return json_operate(a, b, JSON_APPEND);
}

/* Predeclaration */
static int json_do_check_equality(json_t a, json_t b, int null_eq);

/**
 * See if a disjoin is equal to another value.
 **/
static int
json_check_disjoin_equals(json_t disjoin, json_t other)
{
	size_t i;

	for (i = 0; i < disjoin.v->count; i++)
		if (json_do_check_equality(disjoin.v->items[i], other, 0))
			return 1;

	return 0;
}

/**
 * Check if two lists are equal.
 **/
static int
json_check_lists_equal(json_t a, json_t b)
{
	size_t i;

	for (i = 0; i < a.v->count && i < b.v->count; i++)
		if (! json_check_equality(a.v->items[i], b.v->items[i]))
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
json_object_sort_kvs(json_t obj)
{
	struct kv_pair *kvs = obj.v->kvs;
	size_t count = obj.v->count;

	kv_pair_quicksort(kvs, count);
}

/**
 * Check if two JSON objects are equal.
 **/
static int
json_check_objects_equal(json_t a, json_t b)
{
	size_t a_i = 0;
	size_t b_i = 0;
	int cmp;

	json_object_sort_kvs(a);
	json_object_sort_kvs(b);

	while (a_i < a.v->count && b_i < b.v->count) {
		cmp = strcmp(a.v->kvs[a_i].key, b.v->kvs[b_i].key);

		if (! cmp && ! json_check_equality(a.v->kvs[a_i].value,
						   b.v->kvs[b_i].value)) {
			return 0;
		} else if (cmp < 0 && b.v->type != JSON_UOBJECT) {
			return 0;
		} else if (cmp > 0 && a.v->type != JSON_UOBJECT) {
			return 0;
		}

		if (cmp <= 0)
			a_i++;

		if (cmp >= 0)
			b_i++;
	}

	for (; a_i < a.v->count; a_i++) {
		if (b.v->type == JSON_UOBJECT)
			return 1;

		if (! IS_NULL(a.v->kvs[a_i].value))
			return 0;
	}

	for (; b_i < b.v->count; b_i++) {
		if (a.v->type == JSON_UOBJECT)
			return 1;

		if (! IS_NULL(b.v->kvs[b_i].value))
			return 0;
	}

	return 1;
}

/**
 * Distribute an operator through a disjoin on the left-hand side.
 **/
static json_t
json_distribute_left(json_t disjoin, json_t operand, json_type_t operator)
{
	json_t ret;
	size_t i;

	ret.v = xmalloc(sizeof(struct value_data));

	ret.v->type = JSON_DISJOIN;
	ret.v->items = xcalloc(disjoin.v->count, sizeof(json_t));
	ret.v->count = disjoin.v->count;

	for (i = 0; i < ret.v->count; i++)
		ret.v->items[i] = json_operate(disjoin.v->items[i], operand,
					       operator);

	return ret;
}

/**
 * Distribute an operator through a disjoin on the right-hand side.
 **/
static json_t
json_distribute_right(json_t operand, json_t disjoin, json_type_t operator)
{
	json_t ret;
	size_t i;

	ret.v = xmalloc(sizeof(struct value_data));

	ret.v->type = JSON_DISJOIN;
	ret.v->items = xcalloc(disjoin.v->count, sizeof(json_t));
	ret.v->count = disjoin.v->count;

	for (i = 0; i < ret.v->count; i++)
		ret.v->items[i] = json_operate(operand, disjoin.v->items[i],
					       operator);

	return ret;
}

/**
 * Reduce an overlap of two objects.
 **/
static json_t
json_reduce_object_overlap(json_t a, json_t b)
{
	size_t a_i = 0;
	size_t b_i = 0;
	size_t ret_i = 0;
	int cmp;
	json_t ret;

	json_object_sort_kvs(a);
	json_object_sort_kvs(b);

	if (a.v->type == JSON_UOBJECT && b.v->type == JSON_UOBJECT)
		ret.v->type = JSON_UOBJECT;
	else
		ret.v->type = JSON_OBJECT;

	ret.v->count = a.v->count + b.v->count;
	ret.v->kvs = xcalloc(ret.v->count, sizeof(struct kv_pair));

	while (a_i < a.v->count && b_i < b.v->count) {
		cmp = strcmp(a.v->kvs[a_i].key, b.v->kvs[b_i].key);

		if (! cmp) {
			ret.v->kvs[ret_i].key = xstrdup(a.v->kvs[a_i].key);
			ret.v->kvs[ret_i].value = json_overlap(
			       a.v->kvs[a_i].value, b.v->kvs[b_i].value);
			ret_i++;
		} else if (cmp < 0 && b.v->type == JSON_UOBJECT) {
			ret.v->kvs[ret_i].key = xstrdup(a.v->kvs[a_i].key);
			ret.v->kvs[ret_i].value = a.v->kvs[a_i].value;
			ret_i++;
		} else if (cmp > 0 && a.v->type == JSON_UOBJECT) {
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
		if (b.v->type != JSON_UOBJECT)
			continue;

		ret.v->kvs[ret_i].key = xstrdup(a.v->kvs[a_i].key);
		ret.v->kvs[ret_i].value = a.v->kvs[a_i].value;
		ret_i++;
	}

	for (; b_i < b.v->count; b_i++) {
		if (a.v->type != JSON_UOBJECT)
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
static json_t
json_reduce_list_overlap(json_t a, json_t b)
{
	size_t i = 0;
	json_t ret;
	size_t count = a.v->count;
	json_t a_sub;
	json_t b_sub;

	if (b.v->count > count)
		count = b.v->count;

	ret.v->type = JSON_LIST;
	ret.v->items = xcalloc(count, sizeof(json_t));
	ret.v->count = count;

	for (; i < count; i++) {
		a_sub = b_sub = VALUE_NULL;

		if (i < a.v->count)
			a_sub = a.v->items[i];

		if (i < b.v->count)
			b_sub = b.v->items[i];

		ret.v->items[i] = json_overlap(a_sub, b_sub);
	}

	return ret;
}

/**
 * Reduce an overlap of two non-disjoin values.
 **/
static json_t
json_reduce_overlap(json_t a, json_t b)
{
	if (IS_OBJECT(a) && IS_OBJECT(b))
		return json_reduce_object_overlap(a, b);

	if (a.v->type == JSON_LIST && b.v->type == JSON_LIST)
		return json_reduce_list_overlap(a, b);

	if (json_check_equality(a, b))
		return a;

	return VALUE_NULL;
}

/**
 * Reduce a query of two non-disjoin values.
 **/
static json_t
json_reduce_query(json_t a, json_t b)
{
	json_t ret = json_reduce_overlap(a, b);

	if (json_check_represented_in(ret, b))
		return ret;

	return VALUE_NULL;
}

/**
 * Reduce a intersect of two non-disjoin values.
 **/
static json_t
json_reduce_intersect(json_t a, json_t b)
{
	json_t ret = json_reduce_query(a, b);

	if (json_check_represented_in(ret, a))
		return ret;

	return VALUE_NULL;
}

/**
 * Reduce an append operator between two lists.
 **/
static json_t
json_reduce_list_append(json_t a, json_t b)
{
	json_t ret;

	ret.v = xmalloc(sizeof(struct value_data));

	ret.v->count = a.v->count + b.v->count;
	ret.v->type = JSON_LIST;
	ret.v->items = xcalloc(ret.v->count, sizeof(json_t));

	memcpy(ret.v->items, a.v->items, a.v->count * sizeof(json_t));
	memcpy(ret.v->items + a.v->count, b.v->items,
	       b.v->count * sizeof(json_t));

	return ret;
}

/**
 * Reduce an append of two objects.
 **/
static json_t
json_reduce_object_append(json_t a, json_t b)
{
	size_t a_i = 0;
	size_t b_i = 0;
	size_t ret_i = 0;
	int cmp;
	json_t ret;

	json_object_sort_kvs(a);
	json_object_sort_kvs(b);

	if (a.v->type == JSON_UOBJECT || b.v->type == JSON_UOBJECT)
		ret.v->type = JSON_UOBJECT;
	else
		ret.v->type = JSON_OBJECT;

	ret.v->count = a.v->count + b.v->count;
	ret.v->kvs = xcalloc(ret.v->count, sizeof(struct kv_pair));

	while (a_i < a.v->count && b_i < b.v->count) {
		cmp = strcmp(a.v->kvs[a_i].key, b.v->kvs[b_i].key);

		if (! cmp) {
			ret.v->kvs[ret_i].key = xstrdup(a.v->kvs[a_i].key);
			ret.v->kvs[ret_i].value = json_intersect(
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
static json_t json_simplify_transform(json_t in);

/**
 * Reduce an append of two non-disjoin values.
 **/
static json_t
json_reduce_append(json_t a, json_t b)
{
	json_simplify_transform(a);
	json_simplify_transform(b);

	if (IS_OBJECT(a) && IS_OBJECT(b))
		return json_reduce_object_append(a, b);

	if (a.v->type == JSON_LIST && b.v->type == JSON_LIST)
		return json_reduce_list_append(a, b);

	return VALUE_NULL;
}

/**
 * Reduce a JSON value so it is not expressed, at the top level, as a query,
 * intersect, overlap, or append.
 **/
static json_t
json_simplify_transform(json_t in)
{
	switch (in.v->type) {
	case JSON_OVERLAP:
	case JSON_QUERY:
	case JSON_INTERSECT:
	case JSON_APPEND:
		break;
	default:
		return in;
	};

	in.v->items[0] = json_simplify_transform(in.v->items[0]);
	in.v->items[1] = json_simplify_transform(in.v->items[1]);

	if (in.v->items[0].v->type == JSON_DISJOIN)
		return json_distribute_left(in.v->items[0], in.v->items[1],
					    in.v->type);

	if (in.v->items[1].v->type == JSON_DISJOIN)
		return json_distribute_right(in.v->items[0], in.v->items[1],
					     in.v->type);

	switch (in.v->type) {
	case JSON_OVERLAP:
		return json_reduce_overlap(in.v->items[0], in.v->items[1]);
	case JSON_QUERY:
		return json_reduce_query(in.v->items[0], in.v->items[1]);
	case JSON_INTERSECT:
		return json_reduce_intersect(in.v->items[0], in.v->items[1]);
	case JSON_APPEND:
		return json_reduce_append(in.v->items[0], in.v->items[1]);
	default:
		errx(1, "Unreachable statement at %s:%d", __FILE__, __LINE__);
	};
}

/**
 * Check equality of two JSON values. If null_eq is 0, NULL != STRONG_NULL
 **/
int
json_do_check_equality(json_t a, json_t b, int null_eq)
{
	a = json_simplify_transform(a);
	b = json_simplify_transform(b);

	if (a.v->type == JSON_UNIVERSE || b.v->type == JSON_UNIVERSE)
		return 1;

	if (a.v->type == JSON_WILD && ! IS_NULL(b))
		return 1;

	if (b.v->type == JSON_WILD && ! IS_NULL(a))
		return 1;

	if (a.v->type == JSON_WILD || b.v->type == JSON_WILD)
		return 0;

	if (a.v->type == JSON_DISJOIN)
		return json_check_disjoin_equals(a, b);
	
	if (b.v->type == JSON_DISJOIN)
		return json_check_disjoin_equals(b, a);

	if (IS_OBJECT(a) && IS_OBJECT(b))
		return json_check_objects_equal(a, b);

	if (IS_NULL(a) && b.v->type == JSON_STRONG_NULL)
		return 1;

	if (IS_NULL(b) && a.v->type == JSON_STRONG_NULL)
		return 1;

	if (a.v->type != b.v->type)
		return 0;

	if (IS_NULL(a))
		return null_eq;

	if (IS_NULL(b))
		return json_check_lists_equal(a, b);

	return a.v->n == b.v->n;
}

/**
 * Check equality of two JSON values.
 **/
int
json_check_equality(json_t a, json_t b)
{
	return json_do_check_equality(a, b, 1);
}

/**
 * Ensure a JSON object has no indeterminate values.
 **/
static json_t
json_flatten(json_t value)
{
	size_t i;
	size_t j;
	size_t k;
	size_t count;
	json_t ret;

	value = json_simplify_transform(value);

	switch (value.v->type) {
	case JSON_NUMERIC:
	case JSON_NULL:
	case JSON_STRONG_NULL:
	case JSON_WILD:
	case JSON_UNIVERSE:
		return value;
	case JSON_LIST:
		return json_flatten_list(value);
	case JSON_OBJECT:
		return json_flatten_object(value);
	case JSON_DISJOIN:
		break;
	default:
		errx(1, "Unreachable statement at %s:%d", __FILE__, __LINE__);
	};

	for (i = 0; i < value.v->count; i++) {
		value.v->items[i] = json_flatten(value.v->items[i]);
	}

	count = 0;
	for (i = 0; i < value.v->count; i++) {
		if (value.v->type == JSON_DISJOIN)
			count += value.v->count;
		else
			count += 1;
	}

	if (count == value.v->count)
		return value;

	ret.v = xmalloc(sizeof(struct value_data));

	ret.v->type = JSON_DISJOIN;
	ret.v->items = xcalloc(count, sizeof(json_t));
	ret.v->count = count;

	k = 0;
	for (i = 0; i < value.v->count; i++) {
		if (value.v->items[i].v->type != JSON_DISJOIN) {
			ret.v->items[k++] = value.v->items[i];
		} else if (value.v->items[i].v->type != JSON_NULL) {
			for (j = 0; j < value.v->items[i].v->count; j++)
				ret.v->items[k++] =
					value.v->items[i].v->items[j];
		}
	}

	return ret;
}

/**
 * Check whether JSON value a is represented in b.
 **/
int
json_check_represented_in(json_t a, json_t b)
{
	size_t i;

	a = json_flatten(a);
	b = json_simplify_transform(b);

	if (a.v->type == JSON_DISJOIN) {
		for (i = 0; i < a.v->count; i++)
			if (! json_check_represented_in(a.v->items[i], b))
				return 0;

		return 1;
	}

	if (b.v->type == JSON_DISJOIN) {
		for (i = 0; i < b.v->count; i++)
			if (json_check_represented_in(a, b.v->items[i]))
				return 1;

		return 0;
	}

	if (b.v->type == JSON_UNIVERSE)
		return 1;

	if (b.v->type == JSON_WILD)
		return a.v->type != JSON_NULL;

	return json_check_equality(a, b);
}

/**
 * Check whether a and b are corepresentative.
 **/
int
json_check_corepresented(json_t a, json_t b)
{
	/* FIXME: This can be made quicker */
	if (! json_check_represented_in(a, b))
		return 0;
	if (! json_check_represented_in(b, a))
		return 0;
	return 1;
}

/**
 * Get the type of a JSON value.
 **/
json_type_t
json_type(json_t value)
{
	return value.v->type;
}
