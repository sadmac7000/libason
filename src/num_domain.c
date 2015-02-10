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
 * A set of real numbers defined in ranges. Each item in the array is either an
 * endpoint in an interval or a blip. The states array contains 2 bits per
 * item, where if both bits are set the item is an inclusive endpoint, if both
 * are clear it is an inclusive endpoint, and if only one is set, the item is a
 * blip (single value gap in the range). The minus_inf field indicates that
 * minus infinity is in the set (so the first interval item is an endpoint to an
 * interval starting at negative infinity, or a blip). Positive infinity is in
 * the set if we end on an unclosed interval.
 **/
struct ason_num_dom {
	int64_t *items;
	uint64_t *states;
	size_t count;
	int inv_bits;
	int minus_inf;
};

/**
 * Allocate a new, empty number domain.
 **/
static ason_num_dom_t *
ason_num_dom_alloc(void)
{
	return xcalloc(1, sizeof(ason_num_dom_t));
}

/**
 * Invert the meaning of the set.
 */
void
ason_num_dom_invert(ason_num_dom_t *dom)
{
	dom->minus_inf = !dom->minus_inf;
	dom->inv_bits = (~dom->inv_bits) & 3;
}

/**
 * Set a two-bit pair in a field of two-bit pairs.
 **/
#define TWOBIT_SET(_ptr, _pos, _val) ({	\
	uint64_t *ptr = _ptr;		\
	int val = _val;			\
	size_t pos = _pos;		\
\
	ptr += pos / 32;		\
	pos %= 32;			\
\
	*ptr &= ~(3 << pos);		\
	*ptr |= val << pos;		\
})

/**
 * Get a two-bit pair in a field of two-bit pairs.
 **/
#define TWOBIT_GET(_ptr, _pos) ({	\
	uint64_t *ptr = _ptr;		\
	size_t pos = _pos;		\
\
	ptr += pos / 32;		\
	pos %= 32;			\
\
	((*ptr) >> pos) & 3;		\
})

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
	int mode_a = a->minus_inf;
	int mode_b = b->minus_inf;
	int state_a;
	int state_b;
	ason_num_dom_t *ret = ason_num_dom_alloc();
	size_t i, j, k;

	ret->items = xcalloc(a->count + b->count, 8);
	ret->states = xcalloc((a->count + b->count + 31) / 32, 8);

	for (i = j = k = 0; i < a->count && j < b->count;) {
		state_a = TWOBIT_GET(a->states, i) ^ a->inv_bits;
		state_b = TWOBIT_GET(b->states, j) ^ b->inv_bits;

		if (mode_a && mode_b) {
			if (a->items[i] != b->items[i])
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
			if (a->items[i] > b->items[j])
				goto cont;

			if (a->items[i] < b->items[j] ||
				   state_b == 0 || state_a == 3) {
				TWOBIT_SET(ret->states, k, state_a);
			} else if (state_a == 0) {
				TWOBIT_SET(ret->states, k, 3);
			}

			ret->items[k++] = a->items[i];
		} else if (mode_b) {
			if (a->items[i] < b->items[j])
				goto cont;

			if (a->items[i] > b->items[j] ||
				   state_a == 0 || state_b == 3) {
				TWOBIT_SET(ret->states, k, state_b);
			} else if (state_b == 0) {
				TWOBIT_SET(ret->states, k, 3);
			}

			ret->items[k++] = b->items[i];
		}

cont:
		if (a->items[i] <= b->items[j]) {
			mode_a = (!!(state_a % 3)) ^ !!mode_a;
			i++;
		}

		if (a->items[i] >= b->items[j]) {
			mode_b = (!!(state_b % 3)) ^ !!mode_b;
			j++;
		}
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
	ason_num_dom_invert(a);
	ason_num_dom_invert(b);

	ason_num_dom_t *ret = ason_num_dom_union(a, b);

	ason_num_dom_invert(a);
	ason_num_dom_invert(b);
	ason_num_dom_invert(ret);

	return ret;
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
