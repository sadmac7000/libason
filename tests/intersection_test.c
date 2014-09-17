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

#include <stdio.h>
#include <stdlib.h>

#include <ason/value.h>
#include <ason/output.h>
#include <ason/read.h>

#include "harness.h"

TESTS(2);

/**
 * Basic exercise of the parser.
 **/
TEST_MAIN("Intersection")
{
	TEST_INIT();
	ason_t *a;
	ason_t *b;
	ason_t *c;
	ason_t *d;

	a = ason_create_number(6);
	b = ason_create_number(7);
	c = ason_union_d(a, b);
	a = ason_create_number(8);
	b = ason_create_number(9);
	d = ason_union_d(a, b);
	c = ason_union_d(c, d);
	a = ason_create_number(6);
	b = ason_create_number(9);
	d = ason_union_d(a, b);
	c = ason_intersect(d, c);

	TEST("Union Intersection") {
		REQUIRE(ason_check_equal(c, d));
	}

	ason_destroy(c);
	ason_destroy(d);

	TEST("Parsed Union Intersection") {
		a = ason_create_number(6);
		b = ason_read("(6 | 7 | 8 | 9) & (5 | 6)", NULL);
		REQUIRE(ason_check_equal(a, b));
	}

	return 0;
}

