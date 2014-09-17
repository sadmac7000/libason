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

#include "../src/value.h"

#include "harness.h"

TESTS(9);

/**
 * Basic exercise of the parser.
 **/
TEST_MAIN("Correctly determine order")
{
	TEST_INIT();
	ason_t *a;


	a = ason_read("6", NULL);
	TEST("Order 0 integer") {
		REQUIRE(ason_reduce(a) == 0);
	}
	ason_destroy(a);

	a = ason_read("true", NULL);
	TEST("Order 0 boolean") {
		REQUIRE(ason_reduce(a) == 0);
	}
	ason_destroy(a);

	a = ason_read("\"bob\"", NULL);
	TEST("Order 0 string") {
		REQUIRE(ason_reduce(a) == 0);
	}
	ason_destroy(a);

	a = ason_read("{ \"foo\": 6 }", NULL);
	TEST("Order 0 object") {
		REQUIRE(ason_reduce(a) == 0);
	}
	ason_destroy(a);

	a = ason_read("{ \"foo\": { \"bar\": 6 } }", NULL);
	TEST("Order 0 nested object") {
		REQUIRE(ason_reduce(a) == 0);
	}
	ason_destroy(a);

	a = ason_read("[ 6, 7, 8 ]", NULL);
	TEST("Order 0 list") {
		REQUIRE(ason_reduce(a) == 0);
	}
	ason_destroy(a);

	a = ason_read("[ 6, 7, [ 8, 9 ] ]", NULL);
	TEST("Order 0 nested list") {
		REQUIRE(ason_reduce(a) == 0);
	}
	ason_destroy(a);

	a = ason_read("[ 6, 7, { \"foo\": 9 } ]", NULL);
	TEST("Order 0 list with object") {
		REQUIRE(ason_reduce(a) == 0);
	}
	ason_destroy(a);

	a = ason_read("{ \"bar\": 7, \"foo\": [8, 9] }", NULL);
	TEST("Order 0 object with list") {
		REQUIRE(ason_reduce(a) == 0);
	}
	ason_destroy(a);

	return 0;
}

