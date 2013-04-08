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
API_EXPORT ason_t * const VALUE_EMPTY = &VALUE_EMPTY_DATA;

static struct ason VALUE_NULL_DATA = {
	.type = ASON_NULL,
	.items = NULL,
	.count = 0,
};
API_EXPORT ason_t * const VALUE_NULL = &VALUE_NULL_DATA;

static struct ason VALUE_UNIVERSE_DATA = {
	.type = ASON_UNIVERSE,
	.items = NULL,
	.count = 0,
};
API_EXPORT ason_t * const VALUE_UNIVERSE = &VALUE_UNIVERSE_DATA;

static struct ason VALUE_TRUE_DATA = {
	.type = ASON_TRUE,
	.items = NULL,
	.count = 0,
};
API_EXPORT ason_t * const VALUE_TRUE = &VALUE_TRUE_DATA;

static struct ason VALUE_FALSE_DATA = {
	.type = ASON_FALSE,
	.items = NULL,
	.count = 0,
};
API_EXPORT ason_t * const VALUE_FALSE = &VALUE_FALSE_DATA;

static struct ason VALUE_WILD_DATA = {
	.type = ASON_WILD,
	.items = NULL,
	.count = 0,
};
API_EXPORT ason_t * const VALUE_WILD = &VALUE_WILD_DATA;

static struct ason VALUE_OBJ_ANY_DATA = {
	.type = ASON_UOBJECT,
	.items = NULL,
	.count = 0,
};
API_EXPORT ason_t * const VALUE_OBJ_ANY = &VALUE_OBJ_ANY_DATA;

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
 * Create an ASON string value.
 **/
API_EXPORT ason_t *
ason_create_string(const char *string)
{
	ason_t *ret = ason_create(ASON_STRING, 0);
	ret->string = string_to_utf8(string);
	return ret;
}

/**
 * Copy an ASON value.
 **/
API_EXPORT ason_t *
ason_copy(ason_t *a)
{
	if (a == VALUE_EMPTY	||
	    a == VALUE_NULL	||
	    a == VALUE_UNIVERSE	||
	    a == VALUE_WILD	||
	    a == VALUE_TRUE	||
	    a == VALUE_FALSE	||
	    a == VALUE_OBJ_ANY)
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

	if (a == VALUE_EMPTY	||
	    a == VALUE_NULL	||
	    a == VALUE_UNIVERSE	||
	    a == VALUE_WILD	||
	    a == VALUE_OBJ_ANY	||
	    a == VALUE_TRUE	||
	    a == VALUE_FALSE	||
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
	ason_t *ret = ason_create(ASON_NUMERIC, 0);

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
		return ason_create(ASON_LIST, 0);

	ret = ason_create(ASON_LIST, 1);
	ret->items[0] = ason_copy(content);

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
API_EXPORT ason_t *
ason_union(ason_t *a, ason_t *b)
{
	return ason_operate(a, b, ASON_UNION);
}

/**
 * Intersect two ASON values.
 **/
API_EXPORT ason_t *
ason_intersect(ason_t *a, ason_t *b)
{
	return ason_operate(a, b, ASON_INTERSECT);
}

/**
 * Appent ASON value b to a.
 **/
API_EXPORT ason_t *
ason_append(ason_t *a, ason_t *b)
{
	return ason_operate(a, b, ASON_APPEND);
}

/**
 * Complement an ASON value a.
 **/
API_EXPORT ason_t *
ason_complement(ason_t *a)
{
	ason_t *ret = ason_create(ASON_COMP, 1);
	ret->items[0] = ason_copy(a);
	return ret;
}

/**
 * Distribute an operator through a union on the left-hand side.
 **/
static void
ason_distribute_left(ason_t *operator)
{
	ason_t *left = operator->items[0];
	ason_t *right = operator->items[1];
	ason_type_t type = operator->type;
	size_t i;

	free(operator->items);
	operator->items = xmemdup(left->items,
				  left->count * sizeof(ason_t *));
	operator->count = left->count;
	operator->type = ASON_UNION;

	ason_destroy(left);

	for (i = 0; i < operator->count; i++)
		operator->items[i] = ason_operate(operator->items[i], right,
						  type);

	ason_destroy(right);
}

/**
 * Distribute an operator through a union on the right-hand side.
 **/
static void
ason_distribute_right(ason_t *operator)
{
	ason_t *left = operator->items[0];
	ason_t *right = operator->items[1];
	ason_type_t type = operator->type;
	size_t i;

	free(operator->items);
	operator->items = xmemdup(right->items,
				  right->count * sizeof(ason_t *));
	operator->count = right->count;
	operator->type = ASON_UNION;

	ason_destroy(right);

	for (i = 0; i < operator->count; i++)
		operator->items[i] = ason_operate(left, operator->items[i],
						  type);

	ason_destroy(left);
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
	a->type = ASON_EMPTY;
}

/**
 * Alter the value of an ASON value to that of another value.
 **/
static void
ason_clone_into(ason_t *target, ason_t *src)
{
	size_t refcount = target->refcount;
	size_t i;

	ason_make_empty(target);

	memcpy(target, src, sizeof(ason_t));
	target->refcount = refcount;

	if (! target->count)
		return;

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
static int ason_reduce(ason_t *a);

/**
 * Reduce a complement.
 **/
static void
ason_reduce_complement(ason_t *a)
{
	ason_t *tmp;

	if (ason_reduce(a->items[0])) {
		ason_destroy(a->items[0]);
		free(a->items);
		a->count = 0;
		a->type = ASON_UNIVERSE;
	} else if (a->items[0]->type == ASON_UNIVERSE) {
		ason_make_empty(a);
	} else if (a->items[0]->type == ASON_COMP) {
		tmp = ason_copy(a->items[0]->items[0]);
		ason_clone_into_d(a, tmp);
	}
}

/**
 * Distribute an ASON operator through a union. Return whether distribution was
 * possible.
 **/
static int
ason_distribute(ason_t *a)
{
	if (a->items[0]->type == ASON_UNION) {
		ason_distribute_left(a);
	} else if (a->items[1]->type == ASON_UNION) {
		ason_distribute_right(a);
	} else {
		return 0;
	}

	return 1;
}

/**
 * Reduce an intersect of two objects.
 **/
static void
ason_reduce_object_intersect(ason_t *a)
{
	struct ason_coiterator iter;
	ason_t *left;
	ason_t *right;
	ason_t *tmp;
	const char *key;
	ason_type_t type = ASON_OBJECT;
	struct kv_pair *buf = xcalloc(a->items[0]->count + a->items[1]->count,
				      sizeof(struct kv_pair));

	if (a->items[0]->type == ASON_UOBJECT &&
	    a->items[1]->type == ASON_UOBJECT)
		type = ASON_UOBJECT;

	ason_coiterator_init(&iter, a->items[0], a->items[1]);
	ason_make_empty(a);
	a->type = type;
	a->kvs = buf;


	a->count = 0;

	while((key = ason_coiterator_next(&iter, &left, &right))) {
		tmp = ason_intersect(left, right);

		if (ason_reduce(tmp)) {
			ason_destroy(tmp);
			ason_make_empty(a);
			ason_coiterator_release(&iter);
			return;
		}

		a->kvs[a->count].key = xstrdup(key);
		a->kvs[a->count].value = tmp;
		a->count++;
	}

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

	if (a->items[0]->count != a->items[1]->count) {
		ason_make_empty(a);
		return;
	}

	left = ason_copy(a->items[0]);
	right = ason_copy(a->items[1]);
	ason_make_empty(a);

	a->type = ASON_LIST;
	a->items = xcalloc(left->count, sizeof(ason_t *));

	for (a->count = 0; a->count < left->count; a->count++) {
		tmp = ason_intersect(left->items[a->count],
				     right->items[a->count]);

		if (ason_reduce(tmp)) {
			ason_destroy(tmp);
			ason_make_empty(a);
			return;
		}

		a->items[a->count] = tmp;
	}
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

	if (ason_reduce(a->items[0]) || ason_reduce(a->items[1])) {
		ason_make_empty(a);
	} else if (ason_distribute(a)) {
		ason_reduce(a);
	} else if (IS_OBJECT(a->items[0]) && IS_OBJECT(a->items[1])) {
		ason_reduce_object_intersect(a);
	} else if (a->items[0]->type == ASON_NULL) {
		other = a->items[1]->type;
		ason_make_empty(a);

		if (other == ASON_NULL || other == ASON_UNIVERSE)
			a->type = ASON_NULL;
	} else if (a->items[0]->type == ASON_UNIVERSE ||
	    (a->items[0]->type == ASON_WILD &&
	     a->items[1]->type != ASON_NULL)) {
		tmp = ason_copy(a->items[1]);
		ason_clone_into_d(a, tmp);
	} else if (a->items[0]->type != a->items[1]->type) {
		ason_make_empty(a);
	} else if (a->items[0]->type == ASON_TRUE ||
		   a->items[0]->type == ASON_FALSE) {
		other = a->items[0]->type;
		ason_make_empty(a);
		a->type = other;
	} else if (a->items[0]->type == ASON_STRING) {
		string = NULL;

		if (! strcmp(a->items[0]->string, a->items[1]->string))
			string = xstrdup(a->items[0]->string);

		ason_make_empty(a);

		if (string) {
			a->string = string;
			a->type = ASON_STRING;
		}
	} else if (a->items[0]->type == ASON_NUMERIC) {
		n = a->items[1]->n;
		tmp = ason_copy(a->items[0]);

		ason_clone_into_d(a, tmp);
		ason_destroy(tmp);

		if (a->n != n)
			ason_make_empty(a);
	} else if (a->items[0]->type == ASON_LIST) {
		ason_reduce_list_intersect(a);
	} else if (a->items[0]->type == ASON_COMP) {
		tmp = ason_union(a->items[0]->items[0], a->items[1]->items[1]);
		ason_make_empty(a);
		a->type = ASON_COMP;
		a->count = 1;
		a->items = xmalloc(sizeof(ason_t *));
		a->items[0] = tmp;
	} else {
		ason_make_empty(a);
	}
}

/**
 * Reduce an append of two objects.
 **/
static void
ason_reduce_object_append(ason_t *a)
{
	struct ason_coiterator iter;
	const char *key;
	ason_t *left;
	ason_t *right;
	ason_type_t type = ASON_OBJECT;
	struct kv_pair *buf = xcalloc(a->items[0]->count + a->items[1]->count,
				      sizeof(struct kv_pair));

	if (a->items[0]->type == ASON_UOBJECT ||
	    a->items[1]->type == ASON_UOBJECT)
		type = ASON_UOBJECT;

	ason_coiterator_init(&iter, a->items[0], a->items[1]);
	ason_make_empty(a);
	a->type = type;
	a->kvs = buf;
	a->count = 0;

	while ((key = ason_coiterator_next(&iter, &left, &right))) {
		a->kvs[a->count].key = xstrdup(key);

		if (left->type == ASON_NULL) {
			a->kvs[a->count++].value = ason_copy(right);
		} else if (right->type == ASON_NULL) {
			a->kvs[a->count++].value = ason_copy(left);
		} else {
			a->kvs[a->count++].value = ason_intersect(left, right);

			if (ason_reduce(a->kvs[a->count - 1].value)) {
				ason_make_empty(a);
				break;
			}
		}
	}
}

/**
 * Reduce an append of two lists.
 **/
static void
ason_reduce_list_append(ason_t *a)
{
	ason_t *b = ason_copy(a->items[1]);
	ason_t *tmp = ason_copy(a->items[0]);

	ason_clone_into(a, tmp);
	a->items = xrealloc(a->items,
			    (a->count + b->count) * sizeof(ason_t *));

	memcpy(a->items + a->count, b->items, b->count * sizeof(ason_t *));
	a->count += b->count;
}

/**
 * Reduce an append.
 **/
static void
ason_reduce_append(ason_t *a)
{
	if (ason_distribute(a)) {
		ason_reduce(a);
		return;
	}

	ason_reduce(a->items[0]);
	ason_reduce(a->items[1]);

	if (IS_OBJECT(a->items[0]) && IS_OBJECT(a->items[1]))
		ason_reduce_object_append(a);
	else if (a->items[0]->type == ASON_LIST && a->items[1]->type == ASON_LIST)
		ason_reduce_list_append(a);
	else
		ason_make_empty(a);
}

/**
 * Reduce an object.
 **/
static void
ason_reduce_object(ason_t *a)
{
	size_t i;

	for (i = 0; i < a->count; i++) {
		if (ason_reduce(a->kvs[i].value)) {
			ason_make_empty(a);
			break;
		}
	}
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
		if (ason_reduce(a->items[i])) {
			ason_make_empty(a);
			break;
		}
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
	int found_uni;
	int found_wild;
	int found_null;

	for (pos = new_count = 0; pos < a->count; pos++) {
		if (ason_reduce(a->items[pos])) {
			a->items[new_count++] = a->items[pos];
		} else {
			ason_destroy(a->items[pos]);
		}
	}

	a->count = new_count;

	for (pos = 0; pos < a->count; pos++) {
		if (a->items[pos]->type == ASON_UNIVERSE) {
			found_uni = 1;
		} else if (a->items[pos]->type == ASON_WILD) {
			found_wild = 1;
		} else if (a->items[pos]->type == ASON_NULL) {
			found_null = 1;
		}
	}

	found_uni = found_uni || (found_null && found_wild);

	if (found_uni) {
		ason_make_empty(a);
		a->type = ASON_UNIVERSE;
	} else if (! new_count) {
		ason_make_empty(a);
	}
}

/**
 * Reduce an ASON value. This technically mutates the value, but our API allows
 * values to change representation as long as they remain equal to themselves.
 *
 * Return whether the value is now empty.
 **/
static int
ason_reduce(ason_t *a)
{

	if (a->type == ASON_EMPTY)
		return 1;

	if (IS_OBJECT(a))
		ason_reduce_object(a);
	else if (a->type == ASON_LIST)
		ason_reduce_list(a);
	else if (a->type == ASON_UNION)
		ason_reduce_union(a);
	else if (a->type == ASON_INTERSECT)
		ason_reduce_intersect(a);
	else if (a->type == ASON_APPEND)
		ason_reduce_append(a);
	else if (a->type == ASON_COMP)
		ason_reduce_complement(a);

	return a->type == ASON_EMPTY;
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
	ason_t *comp = ason_complement(b);
	int ret = !ason_check_intersects(a, comp);

	ason_destroy(comp);
	return ret;
}

/**
 * Check whether a and b are equal.
 **/
API_EXPORT int
ason_check_equal(ason_t *a, ason_t *b)
{
	/* FIXME: It's possible this can be made quicker */
	if (! ason_check_represented_in(a, b))
		return 0;
	if (! ason_check_represented_in(b, a))
		return 0;
	return 1;
}
