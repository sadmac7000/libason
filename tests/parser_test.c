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

TESTS(21);

/**
 * Basic exercise of the parser.
 **/
TEST_MAIN("Parse values")
{
	ason_t *test_value = NULL;
	ason_t *a;
	ason_t *b = NULL;
	ason_t *c = NULL;
	ason_t *d;
	char *str = NULL;

	a = ason_create_number(6);

	TEST("Parse parameter") {
		b = ason_read("?i", NULL, 6);
		REQUIRE(ason_check_equal(a, b));

		c = ason_read("?i", NULL, 7);
		REQUIRE(!ason_check_equal(a, c));
	}

	ason_destroy(a);
	ason_destroy(b);
	ason_destroy(c);

	a = ason_create_number(~(unsigned int)0);

	TEST("Parse unsigned parameter") {
		b = ason_read("?u", NULL, ~(unsigned int)0);
		REQUIRE(ason_check_equal(a, b));
	}

	ason_destroy(a);
	ason_destroy(b);

	a = ason_create_number(~(uint64_t)0);

	TEST("Parse unsigned I64 parameter") {
		b = ason_read("?U", NULL, ~(uint64_t)0);
		REQUIRE(ason_check_equal(a, b));
	}

	ason_destroy(a);
	ason_destroy(b);

	a = ason_create_number(~(int64_t)0);

	TEST("Parse signed I64 parameter") {
		b = ason_read("?I", NULL, ~(int64_t)0);
		REQUIRE(ason_check_equal(a, b));
	}

	ason_destroy(a);
	ason_destroy(b);

	a = ason_read("6.75", NULL);

	TEST("Parse double parameter") {
		b = ason_read("?F", NULL, (double)6.75);
		REQUIRE(ason_check_equal(a, b));
	}

	ason_destroy(a);
	ason_destroy(b);

	a = ason_read("\"foo\"", NULL);

	TEST("Parse string parameter") {
		b = ason_read("?s", NULL, "foo");
		REQUIRE(ason_check_equal(a, b));
	}

	ason_destroy(a);
	ason_destroy(b);

	a = ason_read("[6,7,[8,9],10]", NULL);
	b = ason_read("[8,9]", NULL);

	TEST("Parse value parameter") {
		c = ason_read("[6,7,?,10]", NULL, b);
		REQUIRE(ason_check_equal(a, c));
	}

	ason_destroy(a);
	ason_destroy(b);
	ason_destroy(c);

	a = ason_create_number(6);
	a = ason_create_object_d("foo", a);

	b = ason_create_number(8);
	b = ason_create_object_d("bar", b);

	a = ason_join_d(a, b);
	b = ason_create_number(98);

	a = ason_union_d(a, b);
	b = ason_create_number(1);
	b = ason_create_list_d(b);

	c = ason_create_number(2);
	c = ason_create_list_d(c);

	b = ason_join_d(b, c);
	c = ason_create_number(3);
	c = ason_create_list_d(c);

	d = ason_join(b, c);
	ason_destroy(c);

	b = ason_intersect_d(b, d);
	a = ason_union_d(a, b);
	
	TEST("Union value") {
		test_value = ason_read("{ \"foo\": 6, \"bar\": 8 } | "
				       "(98 | [1,2,3] & [1,2])", NULL);
		REQUIRE(ason_check_equal(a, test_value));
	}

	ason_destroy(test_value);
	ason_destroy(a);

	TEST("Truth") {
		test_value = ason_read("true", NULL);

		REQUIRE(test_value);
		REQUIRE(ason_check_equal(ASON_TRUE, test_value));
	}

	ason_destroy(test_value);

	TEST("Falsehood") {
		test_value = ason_read("false", NULL);

		REQUIRE(test_value);
		REQUIRE(ason_check_equal(ASON_FALSE, test_value));
	}

	ason_destroy(test_value);

	TEST("String") {
		test_value = ason_read("\"\tstring \\\"☺\\\"\"", NULL);
		a = ason_create_string("\tstring \"☺\"");

		REQUIRE(test_value);
		REQUIRE(ason_check_equal(a, test_value));
	}

	ason_destroy(test_value);
	ason_destroy(a);

	TEST("Number") {
		test_value = ason_read("-6.25", NULL);
		str = ason_asprint(test_value);

		REQUIRE(!strcmp(str, "-6.25"));
	}

	ason_destroy(test_value);
	free(str);
	str = NULL;

	TEST("Equivalence (true)") {
		test_value = ason_read("1 = 1", NULL);

		REQUIRE(ason_check_equal(test_value, ASON_TRUE));
	}

	ason_destroy(test_value);

	TEST("Equivalence (false)") {
		test_value = ason_read("2 = 1", NULL);

		REQUIRE(ason_check_equal(test_value, ASON_FALSE));
	}

	ason_destroy(test_value);

	TEST("Representation (true)") {
		test_value = ason_read("1 in 2 | 1", NULL);

		REQUIRE(ason_check_equal(test_value, ASON_TRUE));
	}

	ason_destroy(test_value);

	TEST("Representation (false)") {
		test_value = ason_read("1 | 2 in 1", NULL);

		REQUIRE(ason_check_equal(test_value, ASON_FALSE));
	}

	ason_destroy(test_value);

	TEST("Unexpected token") {
		REQUIRE(! ason_read("1 | | 2", NULL));
	}

	TEST("Unexpected token (trailing)") {
		REQUIRE(! ason_read("1 | 2 &", NULL));
	}

	TEST("Non-token") {
		REQUIRE(! ason_read("1 % 2", NULL))
	}

	TEST("Non-token (trailing)") {
		REQUIRE(! ason_read("1 | 2 %", NULL))
	}

	a = NULL;
	b = ason_read("[6,7]", NULL);

	TEST("Limited-length read") {
		a = ason_readn("[6,7]@@@@@@", 5, NULL);
		REQUIRE(ason_check_equal(a,b));
	}

	ason_destroy(a);
	ason_destroy(b);

	return 0;
}

