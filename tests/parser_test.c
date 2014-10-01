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

#include <ason/ason.h>
#include <ason/print.h>
#include <ason/read.h>
#include <ason/iter.h>

#include "harness.h"

TESTS(24);

/**
 * Basic exercise of the parser.
 **/
TEST_MAIN("Parse values")
{
	ason_t *test_value = NULL;
	ason_t *a = NULL;
	ason_t *b = NULL;
	ason_t *c = NULL;
	char *str = NULL;
	ason_iter_t *iter;

	TEST("Parse parameter") {
		a = ason_read("?i", 7);
		REQUIRE(ason_type(a) == ASON_TYPE_NUMERIC);
		REQUIRE(ason_long(a) == 7);

		b = ason_read("?i", 6);
		REQUIRE(ason_type(b) == ASON_TYPE_NUMERIC);
		REQUIRE(ason_long(b) == 6);

	}

	ason_destroy(a);
	ason_destroy(b);

	TEST("Parse unsigned parameter") {
		a = ason_read("?u", 3000);
		REQUIRE(ason_type(a) == ASON_TYPE_NUMERIC);
		REQUIRE(ason_long(a) == 3000);
	}

	ason_destroy(a);

	TEST("Parse unsigned I64 parameter") {
		a = ason_read("?I", (uint64_t)3000);
		REQUIRE(ason_type(a) == ASON_TYPE_NUMERIC);
		REQUIRE(ason_long(a) == 3000);
	}

	ason_destroy(a);

	TEST("Parse I64 parameter") {
		a = ason_read("?I", (int64_t)-3000);
		REQUIRE(ason_type(a) == ASON_TYPE_NUMERIC);
		REQUIRE(ason_long(a) == -3000);
	}

	ason_destroy(a);

	TEST("Parse double parameter") {
		a = ason_read("?F", (double)6.75);
		REQUIRE(ason_double(a) == 6.75);
	}

	ason_destroy(a);

	TEST("Parse string parameter") {
		a = ason_read("?s", "foo");
		str = ason_string(a);
		REQUIRE(!strcmp(str, "foo"));
	}

	free(str);
	ason_destroy(a);

	TEST("Parse string parameter list") {
		a = ason_read("[ ?s , ?s ]", "foo", "bar");
		iter = ason_iterate(a);

		REQUIRE(ason_iter_type(iter) == ASON_TYPE_LIST);
		REQUIRE(ason_iter_enter(iter));
		str = ason_iter_string(iter);
		REQUIRE(!strcmp(str, "foo"));
		free(str);
		REQUIRE(ason_iter_next(iter));
		str = ason_iter_string(iter);
		REQUIRE(!strcmp(str, "bar"));
		free(str);
		ason_iter_destroy(iter);
	}

	ason_destroy(a);

	b = ason_read("{*}");
	c = ason_read("6");

	TEST("Parse complex parameter list") {
		a = ason_read("? : { ?s : ? }", b, "foo", c);
		iter = ason_iterate(a);
		ason_destroy(b);
		ason_destroy(c);

		REQUIRE(ason_iter_type(iter) == ASON_TYPE_UOBJECT);
		REQUIRE(ason_iter_enter(iter));
		str = ason_iter_key(iter);
		REQUIRE(!strcmp(str, "foo"));
		free(str);
		REQUIRE(ason_iter_long(iter) == 6);
		REQUIRE(! ason_iter_next(iter));
		REQUIRE(! ason_iter_prev(iter));
		ason_iter_destroy(iter);
	}

	ason_destroy(a);

	a = ason_read("[6,7,[8,9],10]");
	b = ason_read("[8,9]");

	TEST("Parse value parameter") {
		c = ason_read("[6,7,?,10]", b);
		REQUIRE(ason_check_equal(a, c));
	}

	ason_destroy(a);
	ason_destroy(b);
	ason_destroy(c);

	TEST("Union value") {
		int count = 0;
		int subcount = 0;
		char *c;

		a = ason_read("{ \"foo\": 6, \"bar\": 8 } | "
			      "(98 | [1,2,3] & [1,2])");
		iter = ason_iterate(a);

		REQUIRE(ason_iter_type(iter) == ASON_TYPE_UNION);
		REQUIRE(ason_iter_enter(iter));

		do {
			if (ason_iter_type(iter) == ASON_TYPE_NUMERIC) {
				REQUIRE(ason_iter_long(iter) == 98);
				count |= 2;
			} else {
				REQUIRE(ason_iter_type(iter) == ASON_TYPE_OBJECT);
				REQUIRE(ason_iter_enter(iter));

				do {
					c = ason_iter_key(iter);
					REQUIRE(c);

					REQUIRE(ason_iter_type(iter) == ASON_TYPE_NUMERIC);

					if (ason_iter_long(iter) == 6) {
						REQUIRE(!strcmp(c, "foo"));
						subcount |= 1;
					} else {
						REQUIRE(!strcmp(c, "bar"));
						REQUIRE(ason_iter_long(iter) == 8);
						subcount |= 2;
					}
					free(c);
				} while (ason_iter_next(iter));

				REQUIRE(subcount == 3);
				count |= 1;
				ason_iter_exit(iter);
			}
		} while (ason_iter_next(iter));

		REQUIRE(count == 3);
		ason_iter_destroy(iter);
	}

	ason_destroy(a);

	TEST("Truth") {
		test_value = ason_read("true");

		REQUIRE(test_value);
		REQUIRE(ason_check_equal(ASON_TRUE, test_value));
	}

	ason_destroy(test_value);

	TEST("Falsehood") {
		test_value = ason_read("false");

		REQUIRE(test_value);
		REQUIRE(ason_check_equal(ASON_FALSE, test_value));
	}

	ason_destroy(test_value);

	TEST("String") {
		test_value = ason_read("\"\t\001string \\\"☺\\\"\"");
		str = ason_string(test_value);
		REQUIRE(! strcmp(str, "\t\001string \"☺\""));
	}

	free(str);
	ason_destroy(test_value);

	TEST("Number") {
		test_value = ason_read("-6.25");
		str = ason_asprint(test_value);

		REQUIRE(!strcmp(str, "-6.25"));
	}

	ason_destroy(test_value);
	free(str);
	str = NULL;

	TEST("Equivalence (true)") {
		test_value = ason_read("1 = 1");

		REQUIRE(ason_check_equal(test_value, ASON_TRUE));
	}

	ason_destroy(test_value);

	TEST("Equivalence (false)") {
		test_value = ason_read("2 = 1");

		REQUIRE(ason_check_equal(test_value, ASON_FALSE));
	}

	ason_destroy(test_value);

	TEST("Representation (true)") {
		test_value = ason_read("1 in 2 | 1");

		REQUIRE(ason_check_equal(test_value, ASON_TRUE));
	}

	ason_destroy(test_value);

	TEST("Representation (false)") {
		test_value = ason_read("1 | 2 in 1");

		REQUIRE(ason_check_equal(test_value, ASON_FALSE));
	}

	ason_destroy(test_value);

	TEST("Unexpected token") {
		REQUIRE(! ason_read("1 | | 2"));
	}

	TEST("Unexpected token (trailing)") {
		REQUIRE(! ason_read("1 | 2 &"));
	}

	TEST("Non-token") {
		REQUIRE(! ason_read("1 % 2"))
	}

	TEST("Non-token (trailing)") {
		REQUIRE(! ason_read("1 | 2 %"))
	}

	a = NULL;
	b = ason_read("[6,7]");

	TEST("Limited-length read") {
		a = ason_readn("[6,7]@@@@@@", 5);
		REQUIRE(ason_check_equal(a,b));
	}

	ason_destroy(a);
	ason_destroy(b);

	TEST("Empty list") {
		a = ason_read("[]");
		iter = ason_iterate(a);

		REQUIRE(ason_iter_type(iter) == ASON_TYPE_LIST);
		REQUIRE(!ason_iter_enter(iter));

		ason_iter_destroy(iter);
	}

	ason_destroy(a);

	return 0;
}

