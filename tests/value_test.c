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
#include <ason/output.h>

#include "harness.h"

TESTS(4);

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

	TEST("List redistribution") {
		a = ason_read("[6 | 7 | 8, 9]  = "
			      "[6, 9]  | [7, 9]  | [8, 9]", NULL);

		REQUIRE(ason_check_equal(a, ASON_TRUE));
	}

	ason_destroy(a);

	TEST("Splay torture") {
		a = ason_read("[6 | 7 | 8, 9, {\"baz\": 10, "
			      "\"bar\": 11 | 12 | 13, \"bam\": 14, \"foo\": "
			      "15 | 16 | 17}, 18, 19 | 20 | 21] = "
      "[6, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 15 }, 18, 19] |"
      "[7, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 15 }, 18, 19] |"
      "[8, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 15 }, 18, 19] |"
      "[6, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 15 }, 18, 19] |"
      "[7, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 15 }, 18, 19] |"
      "[8, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 15 }, 18, 19] |"
      "[6, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 15 }, 18, 19] |"
      "[7, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 15 }, 18, 19] |"
      "[8, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 15 }, 18, 19] |"
      "[6, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 16 }, 18, 19] |"
      "[7, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 16 }, 18, 19] |"
      "[8, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 16 }, 18, 19] |"
      "[6, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 16 }, 18, 19] |"
      "[7, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 16 }, 18, 19] |"
      "[8, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 16 }, 18, 19] |"
      "[6, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 16 }, 18, 19] |"
      "[7, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 16 }, 18, 19] |"
      "[8, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 16 }, 18, 19] |"
      "[6, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 17 }, 18, 19] |"
      "[7, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 17 }, 18, 19] |"
      "[8, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 17 }, 18, 19] |"
      "[6, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 17 }, 18, 19] |"
      "[7, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 17 }, 18, 19] |"
      "[8, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 17 }, 18, 19] |"
      "[6, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 17 }, 18, 19] |"
      "[7, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 17 }, 18, 19] |"
      "[8, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 17 }, 18, 19] |"
      "[6, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 15 }, 18, 20] |"
      "[7, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 15 }, 18, 20] |"
      "[8, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 15 }, 18, 20] |"
      "[6, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 15 }, 18, 20] |"
      "[7, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 15 }, 18, 20] |"
      "[8, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 15 }, 18, 20] |"
      "[6, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 15 }, 18, 20] |"
      "[7, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 15 }, 18, 20] |"
      "[8, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 15 }, 18, 20] |"
      "[6, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 16 }, 18, 20] |"
      "[7, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 16 }, 18, 20] |"
      "[8, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 16 }, 18, 20] |"
      "[6, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 16 }, 18, 20] |"
      "[7, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 16 }, 18, 20] |"
      "[8, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 16 }, 18, 20] |"
      "[6, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 16 }, 18, 20] |"
      "[7, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 16 }, 18, 20] |"
      "[8, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 16 }, 18, 20] |"
      "[6, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 17 }, 18, 20] |"
      "[7, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 17 }, 18, 20] |"
      "[8, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 17 }, 18, 20] |"
      "[6, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 17 }, 18, 20] |"
      "[7, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 17 }, 18, 20] |"
      "[8, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 17 }, 18, 20] |"
      "[6, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 17 }, 18, 20] |"
      "[7, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 17 }, 18, 20] |"
      "[8, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 17 }, 18, 20] |"
      "[6, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 15 }, 18, 21] |"
      "[7, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 15 }, 18, 21] |"
      "[8, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 15 }, 18, 21] |"
      "[6, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 15 }, 18, 21] |"
      "[7, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 15 }, 18, 21] |"
      "[8, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 15 }, 18, 21] |"
      "[6, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 15 }, 18, 21] |"
      "[7, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 15 }, 18, 21] |"
      "[8, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 15 }, 18, 21] |"
      "[6, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 16 }, 18, 21] |"
      "[7, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 16 }, 18, 21] |"
      "[8, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 16 }, 18, 21] |"
      "[6, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 16 }, 18, 21] |"
      "[7, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 16 }, 18, 21] |"
      "[8, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 16 }, 18, 21] |"
      "[6, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 16 }, 18, 21] |"
      "[7, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 16 }, 18, 21] |"
      "[8, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 16 }, 18, 21] |"
      "[6, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 17 }, 18, 21] |"
      "[7, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 17 }, 18, 21] |"
      "[8, 9, {\"baz\": 10, \"bar\": 11, \"bam\": 14, \"foo\": 17 }, 18, 21] |"
      "[6, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 17 }, 18, 21] |"
      "[7, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 17 }, 18, 21] |"
      "[8, 9, {\"baz\": 10, \"bar\": 12, \"bam\": 14, \"foo\": 17 }, 18, 21] |"
      "[6, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 17 }, 18, 21] |"
      "[7, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 17 }, 18, 21] |"
      "[8, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 17 }, 18, 21]",
      NULL);

		REQUIRE(ason_check_equal(a, ASON_TRUE));
	}

	return 0;
}
