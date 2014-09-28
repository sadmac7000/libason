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
#include <ason/iter.h>

#include "harness.h"

TESTS(22);

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
		a = ason_read("?i", NULL, 7);
		iter = ason_iterate(a);
		REQUIRE(ason_iter_type(iter) == ASON_TYPE_NUMERIC);
		REQUIRE(ason_iter_long(iter) == 7);
		ason_iter_destroy(iter);

		b = ason_read("?i", NULL, 6);
		iter = ason_iterate(b);

		REQUIRE(ason_iter_type(iter) == ASON_TYPE_NUMERIC);
		REQUIRE(ason_iter_long(iter) == 6);

		ason_iter_destroy(iter);
	}

	ason_destroy(a);
	ason_destroy(b);

	TEST("Parse unsigned parameter") {
		a = ason_read("?u", NULL, 3000);
		iter = ason_iterate(a);
		REQUIRE(ason_iter_type(iter) == ASON_TYPE_NUMERIC);
		REQUIRE(ason_iter_long(iter) == 3000);
		ason_iter_destroy(iter);
	}

	ason_destroy(a);

	TEST("Parse unsigned I64 parameter") {
		a = ason_read("?I", NULL, (uint64_t)3000);
		iter = ason_iterate(a);
		REQUIRE(ason_iter_type(iter) == ASON_TYPE_NUMERIC);
		REQUIRE(ason_iter_long(iter) == 3000);
		ason_iter_destroy(iter);
	}

	ason_destroy(a);

	TEST("Parse I64 parameter") {
		a = ason_read("?I", NULL, (int64_t)-3000);
		iter = ason_iterate(a);
		REQUIRE(ason_iter_type(iter) == ASON_TYPE_NUMERIC);
		printf("%lld\n", ason_iter_long(iter));
		REQUIRE(ason_iter_long(iter) == -3000);
		ason_iter_destroy(iter);
	}

	ason_destroy(a);

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

	TEST("Union value") {
		int count = 0;
		int subcount = 0;
		char *c;

		a = ason_read("{ \"foo\": 6, \"bar\": 8 } | "
			      "(98 | [1,2,3] & [1,2])", NULL);
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
		char *c;
		test_value = ason_read("\"\t\001string \\\"☺\\\"\"", NULL);
		iter = ason_iterate(test_value);
		c = ason_iter_string(iter);
		ason_iter_destroy(iter);
		REQUIRE(! strcmp(c, "\t\001string \"☺\""));
		free(c);
	}

	ason_destroy(test_value);

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

	TEST("Empty list") {
		a = ason_read("[]", NULL);
		iter = ason_iterate(a);

		REQUIRE(ason_iter_type(iter) == ASON_TYPE_LIST);
		REQUIRE(!ason_iter_enter(iter));
	}

	ason_destroy(a);

	return 0;
}

