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

#include "iter.h"
#include "util.h"

/**
 * Stack frame for ason iterator.
 **/
struct ason_iter_frame {
	ason_t *value;
	size_t index;
};

/**
 * Begin a new iterator for an ASON value.
 **/
API_EXPORT ason_iter_t *
ason_iterate(ason_t *value)
{
	ason_iter_t *ret = xcalloc(1, sizeof(ason_iter_t));

	ason_reduce(value);
	ret->current = ason_copy(value);

	return ret;
}

/**
 * Set the current field based on the value of the index field.
 **/
static void
ason_iter_index_refresh(ason_iter_t *iter)
{
	ason_t *parent = iter->parents[iter->depth - 1].value;

	if (IS_OBJECT(parent))
		iter->current = parent->kvs[iter->index].value;
	else
		iter->current = parent->items[iter->index];
}

/**
 * Iterate sub-values of the current value.
 **/
API_EXPORT int
ason_iter_enter(ason_iter_t *iter)
{
	size_t alloc_size;

	switch(iter->current->type) {
	case ASON_TYPE_UNION:
	case ASON_TYPE_OBJECT:
	case ASON_TYPE_UOBJECT:
	case ASON_TYPE_LIST:
		break;
	default:
		return 0;
	};

	if (! iter->current->count)
		return 0;

	alloc_size = iter->depth * 2;

	if (iter->depth & (iter->depth - 1))
		alloc_size = 0;

	if (alloc_size < 8)
		alloc_size = 0;

	if (! iter->parents)
		alloc_size = 8;

	if (alloc_size)
		iter->parents =
			xrealloc(iter->parents,
				 alloc_size * sizeof(struct ason_iter_frame));

	iter->parents[iter->depth].value = iter->current;
	iter->parents[iter->depth].index= iter->index;
	iter->depth++;
	iter->index = 0;
	ason_iter_index_refresh(iter);

	return 1;
}

/**
 * Return to the parent value of the current value.
 **/
API_EXPORT int
ason_iter_exit(ason_iter_t *iter)
{
	if (! iter->depth)
		return 0;

	iter->current = iter->parents[--iter->depth].value;
	iter->index = iter->parents[iter->depth].index;

	return 1;
}

/**
 * Get the next successive value for this iterator.
 **/
API_EXPORT int
ason_iter_next(ason_iter_t *iter)
{
	ason_t *parent;

	if (! iter->depth)
		return 0;

	parent = iter->parents[iter->depth - 1].value;

	if ((parent->count - 1) == iter->index)
		return 0;

	iter->index++;
	ason_iter_index_refresh(iter);
	return 1;
}

/**
 * Get the previous value for this iterator.
 **/
API_EXPORT int
ason_iter_prev(ason_iter_t *iter)
{
	if (! iter->depth)
		return 0;
	if (! iter->index)
		return 0;

	iter->index--;
	ason_iter_index_refresh(iter);
	return 1;
}

/**
 * Get the type of the value this iterator points to.
 **/
API_EXPORT ason_type_t
ason_iter_type(ason_iter_t *iter)
{
	return ason_type(iter->current);
}

/**
 * Return the long value of the iterator's current value. Abort if the value is
 * not a number.
 **/
API_EXPORT long long
ason_iter_long(ason_iter_t *iter)
{
	return ason_long(iter->current);
}

/**
 * Return the double value of the iterator's current value. Abort if the value
 * is not a number.
 **/
API_EXPORT double
ason_iter_double(ason_iter_t *iter)
{
	return ason_double(iter->current);
}

/**
 * Return the string value of the iterator's current value. Abort if the value
 * is not a string. The result must be freed by the caller.
 **/
API_EXPORT char *
ason_iter_string(ason_iter_t *iter)
{
	return ason_string(iter->current);
}

/**
 * Return the string value of the key for the iterator's current value. Abort
 * if the current value is not in an object. The result must be freed by the
 * caller.
 **/
API_EXPORT char *
ason_iter_key(ason_iter_t *iter)
{
	ason_t *parent;

	if (! iter->depth)
		errx(1, "Cannot find key for iterator root");

	parent = iter->parents[iter->depth - 1].value;

	if (parent->type != ASON_TYPE_OBJECT &&
	    parent->type != ASON_TYPE_UOBJECT)
		errx(1, "Cannot get key for member of non-object");

	return xstrdup(parent->kvs[iter->index].key);
}

/**
 * Get the current ASON value for this iterator. This creates a new ASON value
 * which must be destroyed.
 **/
API_EXPORT ason_t *
ason_iter_value(ason_iter_t *iter)
{
	return ason_copy(iter->current);
}

/**
 * Destroy an ASON iterator.
 **/
API_EXPORT void
ason_iter_destroy(ason_iter_t *iter)
{
	if (iter->depth)
		ason_destroy(iter->parents[0].value);
	else
		ason_destroy(iter->current);

	free(iter->parents);
	free(iter);
}
