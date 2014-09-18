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
TEST_MAIN("Complementation")
{
	ason_t *a = NULL;
	ason_t *b = NULL;
	ason_t *c = NULL;

	TEST("Comparison/Identity") {
		a = ason_create_number(6);
		b = ason_complement_d(ason_complement(a));
		REQUIRE(ason_check_equal(a, b));
	}

	TEST("Parsing") {
		c = ason_read("!!6", NULL);
		REQUIRE(ason_check_equal(b, c));
		REQUIRE(ason_check_equal(a, c));
	}

	ason_destroy(a);
	ason_destroy(b);
	ason_destroy(c);

	return 0;
}

