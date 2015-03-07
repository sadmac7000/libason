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

	ret->current = ason_copy(value);

	return ret;
}

/**
 * Iterate sub-values of the current value.
 **/
API_EXPORT int
ason_iter_enter(ason_iter_t *iter)
{
	(void)iter;
	return 0;
}

/**
 * Return to the parent value of the current value.
 **/
API_EXPORT int
ason_iter_exit(ason_iter_t *iter)
{
	(void)iter;
	return 0;
}

/**
 * Get the next successive value for this iterator.
 **/
API_EXPORT int
ason_iter_next(ason_iter_t *iter)
{
	(void)iter;
	return 0;
}

/**
 * Get the previous value for this iterator.
 **/
API_EXPORT int
ason_iter_prev(ason_iter_t *iter)
{
	(void)iter;
	return 0;
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
	(void)iter;
	return xstrdup("Key");
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
