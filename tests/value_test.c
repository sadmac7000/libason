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
#include <errno.h>

#include <ason/value.h>
#include <ason/read.h>

#include "harness.h"

TESTS(2);

/**
 * Full exercise of value reduction.
 **/
TEST_MAIN("Value Reduction")
{
	ason_t *a = NULL;

	TEST("Order 3 on order 3 representation") {
		a = ason_read("{\"foo\": 6, \"bar\": !7} in "
			      "{\"foo\": 6, \"bar\": !7 | 8, *}", NULL);

		REQUIRE(ason_check_equal(a, ASON_TRUE));
	}

	ason_destroy(a);

	TEST("Object redistribution") {
		a = ason_read("{\"foo\": 6 | 7 | 8, \"bar\": 9}  = "
			      "{\"foo\": 6, \"bar\": 9}  | "
			      "{\"foo\": 7, \"bar\": 9}  | "
			      "{\"foo\": 8, \"bar\": 9}", NULL);

		REQUIRE(ason_check_equal(a, ASON_TRUE));
	}

	ason_destroy(a);

	return 0;
}
