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

#include <ason/ason.h>
#include <ason/read.h>
#include <ason/print.h>

#include "harness.h"

TESTS(28);

/**
 * Full exercise of value reduction.
 **/
TEST_MAIN("Value Reduction")
{
	TEST_ASON_EXPR("Object redistribution",
		       "{\"foo\": 6 | 7 | 8, \"bar\": 9}  = "
		       "{\"foo\": 6, \"bar\": 9}  | "
		       "{\"foo\": 7, \"bar\": 9}  | "
		       "{\"foo\": 8, \"bar\": 9}");

	TEST_ASON_EXPR("List redistribution",
		       "[6 | 7 | 8, 9]  = [6, 9]  | [7, 9]  | [8, 9]");

	TEST_ASON_EXPR("Splay torture",
		       "[6 | 7 | 8, 9, {\"baz\": 10, \"bar\": 11 | 12 | 13, "
		       "\"bam\": 14, \"foo\": 15 | 16 | 17}, 18, 19 | 20 | "
		       "21] = "
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
      "[8, 9, {\"baz\": 10, \"bar\": 13, \"bam\": 14, \"foo\": 17 }, 18, 21]");

	TEST_ASON_EXPR("List intersect",
		       "[ !5, !6, !7 ] & [ 6, 7, 8 ] = [ 6, 7, 8 ]");

	TEST_ASON_EXPR("Slightly harder list intersect",
		       "[ !5, !6, !7 ] & [ 6, 7, !8 ] = [ 6, 7, !(7 | 8) ]");

	TEST_ASON_EXPR("Length-mismatched list intersect",
		       "[ !5, !6, !7 ] & [ 6, 7, !8, 9 ] = _");

	TEST_ASON_EXPR("Parameter-obliterating list intersect",
		       "[ !5, !6, !7 ] & [ !6, 8, 7 ] = _");

	TEST_ASON_EXPR("Complement-collapsing union",
		       "{ \"foo\": !6 } | !(6|7|8) | !7 = "
		       "{ \"foo\": !6 } | !7 ");

	TEST_ASON_EXPR("Intersecting universal objects",
		       "{ \"foo\": 6, \"bar\": 7 | 8, \"baz\": 8, *} & "
		       "{ \"foo\": 6, \"bar\": 7, \"bam\": 9, *} = "
		       "{ \"foo\": 6, \"bar\": 7, \"baz\": 8, \"bam\": 9, *}");

	TEST_ASON_EXPR("Joining universal objects",
		       "{ \"foo\": 6, \"bar\": null, \"baz\": 8, *} : "
		       "{ \"foo\": 6, \"bar\": 7, \"bam\": 9, *} = "
		       "{ \"foo\": 6, \"bar\": 7, \"baz\": 8, \"bam\": 9, *}");

	TEST_ASON_EXPR("Collapsing order 0/order 3 union",
		       "{\"foo\": !7 } = {\"foo\": 6} | {\"foo\": !7 }");

	TEST_ASON_EXPR("Non-collapsing order 0/order 3 union",
		       "6 in 6 | {\"foo\": !7 }");

	TEST_ASON_EXPR("Simple Intersection of Unions",
		       "(6 | 7 | 8 | 9) & (5 | 6) = 6");

	TEST_ASON_EXPR("0-2 Union", "6 | 7 | !(6|7|8) = !8");

	TEST_ASON_EXPR("0-2 Union with 3 trailer", "6 | 7 | !(6|7|8) | [!9] = !8");

	TEST_ASON_EXPR("0-2 Union with simple complement", "6 | 7 | !6 = U");

	TEST_ASON_EXPR("Order 3 on order 3 representation",
		       "{\"foo\": 6, \"bar\": !7} in "
		       "{\"foo\": 6, \"bar\": !7 | 8, *}");

	TEST_ASON_EXPR("3 on 3 union representation",
		       "{\"foo\": [!(7|8)]} in {\"foo\": [!8]}|[!9]");

	TEST_ASON_EXPR("3 union on 3 representation",
		       "{\"foo\": [!(7|8)]}|[!9] in {\"foo\": [!8]}|[!9]");

	TEST_ASON_EXPR("Order 3 on order 3 complement representation",
		       "!{\"foo\": 6, \"bar\": !7 | 8, *} in "
		       "!{\"foo\": 6, \"bar\": !7}");

	TEST_ASON_EXPR("3 on 3 complement representation",
		       "{\"foo\": !(6|7|[!8])} & !{\"foo\": [!8]} = "
		       "{\"foo\": !(6|7|[!8])}");

	TEST_ASON_EXPR("Mismatched 3 on 3 list complement intersection",
		       "[ 6, !7 ] & ! [ 6, !7, 8 ] = [6, !7]");

	TEST_ASON_EXPR("Col3/Comp3 union intersection",
		       "!( [8 | !9] | 6 ) & [6, !7] = [6, !7]");

	TEST_ASON_EXPR("Union of order 3 complements",
		       "![6,7,!8, 7] | ![6,!9,7, 7] = ![6,7,7, 7]");

	TEST_ASON_EXPR("Order 3 representation in order 3 complement",
		       "[6,7,!8] in ![4,5,!6]");

	TEST_ASON_EXPR("Object append",
		       "{} : {\"foo\":\"bar\"} = {\"foo\":\"bar\"}");

	TEST("Destructor safety") {
		ason_destroy(NULL);
	}

	TEST("Global copy memory safety") {
		ason_copy(ASON_EMPTY);
		ason_copy(ASON_NULL);
		ason_copy(ASON_UNIVERSE);
		ason_copy(ASON_WILD);
		ason_copy(ASON_TRUE);
		ason_copy(ASON_FALSE);
		ason_copy(ASON_OBJ_ANY);
	}

	return 0;
}
