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

#include <ason/ason.h>

#include "value.h"
#include "util.h"
#include "stringfunc.h"

/**
 * Handy value constants.
 **/
static struct ason ASON_EMPTY_DATA = {
	.atoms = 0,
	.num_dom = NULL,
};
API_EXPORT ason_t * const ASON_EMPTY = &ASON_EMPTY_DATA;

static struct ason ASON_NULL_DATA = {
	.atoms = ATOM_NULL,
	.num_dom = NULL,
};
API_EXPORT ason_t * const ASON_NULL = &ASON_NULL_DATA;

static struct ason ASON_UNIVERSE_DATA = {
	.atoms = ATOM_TRUE | ATOM_FALSE | ATOM_NULL,
	.num_dom = &ASON_NUM_DOM_UNIVERSE_DATA,
};
API_EXPORT ason_t * const ASON_UNIVERSE = &ASON_UNIVERSE_DATA;

static struct ason ASON_TRUE_DATA = {
	.atoms = ATOM_TRUE,
	.num_dom = NULL,
};
API_EXPORT ason_t * const ASON_TRUE = &ASON_TRUE_DATA;

static struct ason ASON_FALSE_DATA = {
	.atoms = ATOM_FALSE,
	.num_dom = NULL,
};
API_EXPORT ason_t * const ASON_FALSE = &ASON_FALSE_DATA;

static struct ason ASON_WILD_DATA = {
	.atoms = ATOM_TRUE | ATOM_FALSE,
	.num_dom = &ASON_NUM_DOM_UNIVERSE_DATA,
};
API_EXPORT ason_t * const ASON_WILD = &ASON_WILD_DATA;

static struct ason ASON_OBJ_ANY_DATA = {
	.atoms = 0,
	.num_dom = NULL,
};
API_EXPORT ason_t * const ASON_OBJ_ANY = &ASON_OBJ_ANY_DATA;

/**
 * Create an ASON string value.
 **/
ason_t *
ason_create_string(const char *string)
{
	(void)string;
	return ASON_EMPTY;
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

	free(a);
}

/**
 * Create an ASON numeric value from a fixed point number.
 **/
ason_t *
ason_create_fixnum(int64_t number)
{
	ason_t *ret = xcalloc(1, sizeof(ason_t));
	ret->refcount = 1;

	ret->num_dom = ason_num_dom_create_singleton(number);
	return ret;
}

/**
 * Create an ASON list value.
 **/
ason_t *
ason_create_list(ason_t *content)
{
	(void)content;
	return ASON_EMPTY;
}

/**
 * Create an ASON list with the values in one list as well as the values in
 * another list.
 **/
ason_t *
ason_append_lists(ason_t *a, ason_t *b)
{
	(void)a;
	(void)b;
	return ASON_EMPTY;
}

/**
 * Create an ASON object value.
 **/
ason_t *
ason_create_object(const char *key, ason_t *value) 
{
	(void)key;
	(void)value;
	return ASON_EMPTY;
}

/**
 * Union two ASON values.
 **/
ason_t *
ason_union(ason_t *a, ason_t *b)
{
	ason_t *ret;

	ret = xcalloc(1, sizeof(ason_t));
	ret->refcount = 1;
	ret->num_dom = ason_num_dom_union(a->num_dom, b->num_dom);
	ret->atoms = a->atoms | b->atoms;

	return ret;
}

/**
 * Intersect two ASON values.
 **/
ason_t *
ason_intersect(ason_t *a, ason_t *b)
{
	ason_t *ret;

	ret = xcalloc(1, sizeof(ason_t));
	ret->refcount = 1;
	ret->num_dom = ason_num_dom_intersect(a->num_dom, b->num_dom);
	ret->atoms = a->atoms & b->atoms;

	return ret;
}

/**
 * Join ASON value b to a.
 **/
ason_t *
ason_join(ason_t *a, ason_t *b)
{
	return ason_intersect(a, b);
}

/**
 * Complement an ASON value a.
 **/
ason_t *
ason_complement(ason_t *a)
{
	ason_t *ret;

	ret = xcalloc(1, sizeof(ason_t));
	ret->refcount = 1;
	ret->num_dom = ason_num_dom_invert(a->num_dom);
	ret->atoms = (~a->atoms) & (ATOM_TRUE | ATOM_FALSE | ATOM_NULL);

	return ret;
}

/**
 * A boolean ASON value indicating whether a is represented in b.
 **/
ason_t *
ason_representation_in(ason_t *a, ason_t *b)
{
	if (ason_check_represented_in(a, b))
		return ASON_TRUE;
	else
		return ASON_FALSE;
}

/**
 * A boolean ASON value indicating whether a is equal to b.
 **/
ason_t *
ason_equality(ason_t *a, ason_t *b)
{
	if (ason_check_equal(a, b))
		return ASON_TRUE;
	else
		return ASON_FALSE;
}


/**
 * Check whether ASON value a is represented in b.
 **/
API_EXPORT int
ason_check_represented_in(ason_t *a, ason_t *b)
{
	ason_t *tmp = ason_intersect(a, b);
	int ret = ason_check_equal(a, tmp);

	ason_destroy(tmp);

	return ret;
}

/**
 * Check whether a and b are equal.
 **/
API_EXPORT int
ason_check_equal(ason_t *a, ason_t *b)
{
	return !ason_num_dom_compare(a->num_dom, b->num_dom);
}

/**
 * Get the type of an ASON value.
 **/
API_EXPORT ason_type_t
ason_type(ason_t *a)
{
	(void)a;
	return ASON_TYPE_UNION;
}

/**
 * Get the integral numerical value of an ASON value.
 **/
API_EXPORT long long
ason_long(ason_t *a)
{
	(void)a;
	return 0;
}

/**
 * Get the floating point numerical value of an ASON value.
 **/
API_EXPORT double
ason_double(ason_t *a)
{
	(void)a;
	return 0;
}

/**
 * Get the string value of an ASON value.
 **/
API_EXPORT char *
ason_string(ason_t *a)
{
	(void)a;
	return xstrdup("No implementation");
}
