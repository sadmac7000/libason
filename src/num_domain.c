/**
 * Copyright © 2015 Casey Dahlin <casey.dahlin@gmail.com>
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

#include <string.h>
#include <stdint.h>

#include "num_domain.h"
#include "util.h"

/**
 * A number domain containing all numbers.
 **/
ason_num_dom_t ASON_NUM_DOM_UNIVERSE_DATA = {
	.items = NULL,
	.states = NULL,
	.count = 0,
	.inv_bits = 0,
	.minus_inf = 1,
	.refcount = 0,
};
ason_num_dom_t * const ASON_NUM_DOM_UNIVERSE = &ASON_NUM_DOM_UNIVERSE_DATA;

/**
 * Allocate a new, empty number domain.
 **/
static ason_num_dom_t *
ason_num_dom_alloc(void)
{
	ason_num_dom_t *ret = xcalloc(1, sizeof(ason_num_dom_t));
	ret->refcount = 1;
	return ret;
}

/**
 * Invert the meaning of the set.
 */
ason_num_dom_t *
ason_num_dom_invert(ason_num_dom_t *dom)
{
	if (! dom)
		return ASON_NUM_DOM_UNIVERSE;

	if (dom == ASON_NUM_DOM_UNIVERSE)
		return NULL;

	dom = xmemdup(dom, sizeof(ason_num_dom_t));

	dom->refcount = 1;
	dom->minus_inf = !dom->minus_inf;
	dom->inv_bits = (~dom->inv_bits) & 3;

	return dom;
}

/**
 * Take two state values and union their meanings.
 **/
#define UNION_STATES(a, b) (a == b ? a : 1);

/**
 * Union the set with another.
 **/
ason_num_dom_t *
ason_num_dom_union(ason_num_dom_t *a, ason_num_dom_t *b)
{
	int mode_a;
	int mode_b;
	int state_a;
	int state_b;
	ason_num_dom_t *ret;
	size_t i, j, k;

	if (a == b || ! b)
		return ason_num_dom_copy(a);
	if (! a)
		return ason_num_dom_copy(b);

	mode_a = a->minus_inf;
	mode_b = b->minus_inf;

	if (a == ASON_NUM_DOM_UNIVERSE || b == ASON_NUM_DOM_UNIVERSE)
		return ASON_NUM_DOM_UNIVERSE;

	ret = ason_num_dom_alloc();
	ret->items = xcalloc(a->count + b->count, 8);
	ret->states = xcalloc((a->count + b->count + 31) / 32, 8);

	for (i = j = k = 0; i < a->count && j < b->count;) {
		state_a = TWOBIT_GET(a->states, i) ^ a->inv_bits;
		state_b = TWOBIT_GET(b->states, j) ^ b->inv_bits;

		if (mode_a && mode_b) {
			if (a->items[i] != b->items[j])
				goto cont;
			if (state_a == 3 && (state_b % 3))
				goto cont;
			if (state_b == 3 && (state_a % 3))
				goto cont;

			ret->items[k] = a->items[i];

			if (state_a == state_b)
				TWOBIT_SET(ret->states, k, state_a);
			else
				TWOBIT_SET(ret->states, k, 1);

			k++;
		} else if (! (mode_a || mode_b)) {
			if (a->items[i] == b->items[j]) {
				if (state_a == 3 || state_b == 3) {
					TWOBIT_SET(ret->states, k, 3);
				} else {
					TWOBIT_SET(ret->states, k, state_a && state_b);
				}

				ret->items[k++] = a->items[i];
			} else if (a->items[i] > b->items[j]) {
				TWOBIT_SET(ret->states, k, state_b);
				ret->items[k++] = b->items[j];
			} else {
				TWOBIT_SET(ret->states, k, state_a);
				ret->items[k++] = a->items[i];
			}
		} else if (mode_a) {
			if (a->items[i] >= b->items[j])
				goto cont;

			if (a->items[i] < b->items[j] ||
				   state_b == 0 || state_a == 3) {
				TWOBIT_SET(ret->states, k, state_a);
			} else if (state_a == 0) {
				TWOBIT_SET(ret->states, k, 3);
			}

			ret->items[k++] = a->items[i];
		} else if (mode_b) {
			if (a->items[i] <= b->items[j])
				goto cont;

			if (a->items[i] > b->items[j] ||
				   state_a == 0 || state_b == 3) {
				TWOBIT_SET(ret->states, k, state_b);
			} else if (state_b == 0) {
				TWOBIT_SET(ret->states, k, 3);
			}

			ret->items[k++] = b->items[j];
		}

cont:
		if (a->items[i] < b->items[j]) {
			mode_a = (!(state_a % 3)) ^ !!mode_a;
			i++;
		} else if (a->items[i] > b->items[j]) {
			mode_b = (!(state_b % 3)) ^ !!mode_b;
			j++;
		} else {
			mode_a = (!(state_a % 3)) ^ !!mode_a;
			mode_b = (!(state_b % 3)) ^ !!mode_b;
			i++;
			j++;
		}
	}

	while (i < a->count && ! mode_b) {
		state_a = TWOBIT_GET(a->states, i) ^ a->inv_bits;
		TWOBIT_SET(ret->states, k, state_a);
		ret->items[k++] = a->items[i++];
	}

	while (j < b->count && ! mode_a) {
		state_b = TWOBIT_GET(b->states, j) ^ b->inv_bits;
		TWOBIT_SET(ret->states, k, state_b);
		ret->items[k++] = b->items[j++];
	}

	if (! k) {
		ason_num_dom_destroy(ret);
		return ASON_NUM_DOM_UNIVERSE;
	}

	ret->minus_inf = a->minus_inf || b->minus_inf;
	ret->count = k;
	return ret;
}

/**
 * Intersect two number domains
 **/
ason_num_dom_t *
ason_num_dom_intersect(ason_num_dom_t *a, ason_num_dom_t *b)
{
	ason_num_dom_t *c;

	if (a == NULL || b == NULL)
		return NULL;

	if (a == ASON_NUM_DOM_UNIVERSE)
		return ason_num_dom_copy(b);
	if (b == ASON_NUM_DOM_UNIVERSE)
		return ason_num_dom_copy(a);

	a = ason_num_dom_invert(a);
	b = ason_num_dom_invert(b);
	c = ason_num_dom_union(a, b);

	return ason_num_dom_invert(c);
}

/**
 * Create a new domain with only one item.
 **/
ason_num_dom_t *
ason_num_dom_create_singleton(int64_t item)
{
	ason_num_dom_t *ret = ason_num_dom_alloc();

	ret->items = xcalloc(1, 8);
	ret->states = xcalloc(1, 8);
	ret->items[0] = item;
	ret->count = 1;
	TWOBIT_SET(ret->states, 0, 1);

	return ret;
}

#define RANGE_INCL 3
#define RANGE_EXCL 0

/**
 * Create a new domain with range of items
 **/
ason_num_dom_t *
ason_num_dom_create_range(int64_t a, int64_t b,
			  int int_start, int int_end)
{
	ason_num_dom_t *ret = ason_num_dom_alloc();

	ret->items = xcalloc(2, 8);
	ret->states = xcalloc(1, 8);
	ret->items[0] = a;
	ret->items[1] = b;
	TWOBIT_SET(ret->states, 0, int_start);
	TWOBIT_SET(ret->states, 1, int_end);

	return ret;
}

/**
 * Create a new domain with range of items starting at minus infinity.
 **/
ason_num_dom_t *
ason_num_dom_create_range_from_minus_inf(int64_t stop, int intv)
{
	ason_num_dom_t *ret = ason_num_dom_alloc();

	ret->items = xcalloc(1, 8);
	ret->states = xcalloc(1, 8);
	ret->items[0] = stop;
	ret->minus_inf = 1;
	TWOBIT_SET(ret->states, 0, intv);

	return ret;
}

/**
 * Create a new domain with range of items going to infinity.
 **/
ason_num_dom_t *
ason_num_dom_create_range_to_inf(int64_t start, int intv)
{
	ason_num_dom_t *ret = ason_num_dom_alloc();

	ret->items = xcalloc(1, 8);
	ret->states = xcalloc(1, 8);
	ret->items[0] = start;
	TWOBIT_SET(ret->states, 0, intv);

	return ret;
}

/**
 * Comparison operation for number domains.
 **/
int
ason_num_dom_compare(ason_num_dom_t *a, ason_num_dom_t *b)
{
	size_t i;
	int state_a;
	int state_b;

	if (a == b)
		return 0;

	if (! a)
		return -1;
	if (! b)
		return 1;
	if (a == ASON_NUM_DOM_UNIVERSE)
		return 1;
	if (b == ASON_NUM_DOM_UNIVERSE)
		return -1;

	if (a->minus_inf && ! b->minus_inf)
		return -1;
	if (! a->minus_inf && b->minus_inf)
		return 1;

	for (i = 0; i < a->count && i < b->count; i++) {
		state_a = TWOBIT_GET(a->states, i) ^ a->inv_bits;
		state_b = TWOBIT_GET(b->states, i) ^ b->inv_bits;
		if (a->items[i] < b->items[i])
			return -1;
		if (a->items[i] > b->items[i])
			return 1;

		if (state_a == state_b)
			continue;

		if (state_a == 3 || state_b == 0)
			return 1;
		if (state_b == 3 || state_a == 0)
			return -1;
	}

	if (a->count != b->count)
		return a->count - b->count;

	return 0;
}

/**
 * Destroy a number domain.
 **/
void
ason_num_dom_destroy(ason_num_dom_t *dom)
{
	if (dom == ASON_NUM_DOM_UNIVERSE)
		return;
	if (! dom)
		return;

	if (--dom->refcount)
		return;

	free(dom->items);
	free(dom->states);
	free(dom);
}

/**
 * Copy a number domain
 **/
ason_num_dom_t *
ason_num_dom_copy(ason_num_dom_t *dom)
{
	if (! dom)
		return NULL;

	if (dom != ASON_NUM_DOM_UNIVERSE)
		dom->refcount++;

	return dom;
}
