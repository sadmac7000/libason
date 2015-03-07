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

#ifndef NUM_DOMAIN_H
#define NUM_DOMAIN_H

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
typedef struct ason_num_dom {
	int64_t *items;
	uint64_t *states;
	size_t count;
	int inv_bits;
	int minus_inf;
	size_t refcount;
} ason_num_dom_t;

extern ason_num_dom_t * const ASON_NUM_DOM_UNIVERSE;
extern ason_num_dom_t ASON_NUM_DOM_UNIVERSE_DATA;

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
	*ptr &= ~(3 << (pos * 2));	\
	*ptr |= val << (pos * 2);	\
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
	((*ptr) >> (pos * 2)) & 3;		\
})

#ifdef __cplusplus
extern "C" {
#endif

ason_num_dom_t *ason_num_dom_create_singleton(int64_t item);
ason_num_dom_t *ason_num_dom_create_range(int64_t a, int64_t b,
			  int int_start, int int_end);
ason_num_dom_t *ason_num_dom_create_range_from_minus_inf(int64_t stop,
							 int intv);
ason_num_dom_t *ason_num_dom_create_range_to_inf(int64_t start, int intv);
int ason_num_dom_compare(ason_num_dom_t *a, ason_num_dom_t *b);
void ason_num_dom_destroy(ason_num_dom_t *dom);
ason_num_dom_t *ason_num_dom_union(ason_num_dom_t *a, ason_num_dom_t *b);
ason_num_dom_t *ason_num_dom_intersect(ason_num_dom_t *a, ason_num_dom_t *b);
ason_num_dom_t *ason_num_dom_invert(ason_num_dom_t *dom);
ason_num_dom_t *ason_num_dom_copy(ason_num_dom_t *dom);

#ifdef __cplusplus
}
#endif

#endif /* NUM_DOMAIN_H */
