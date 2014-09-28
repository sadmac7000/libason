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

TESTS(7);

/**
 * Basic exercise of the parser.
 **/
TEST_MAIN("Iteration")
{
	ason_t *a;
	ason_t *b = NULL;
	ason_t *c = NULL;
	char *str;
	ason_iter_t *iter;
	int i;
	double dbl;

	a = ason_read("{ \"foo\": 6, \"bar\": 7, \"baz\": 8 }");
	iter = ason_iterate(a);

	TEST("Object Iteration") {
		struct {
			char name[4];
			int value;
			int seen;
		} items[3] = {
			{ "foo", 6, 0},
			{ "bar", 7, 0},
			{ "baz", 8, 0},
		};


		REQUIRE(ason_iter_type(iter) == ASON_TYPE_OBJECT);
		REQUIRE(ason_iter_enter(iter));

		do {
			for (i = 0; i < 3; i++) {
				str = ason_iter_key(iter);

				if (! strcmp(items[i].name, str)) {
					REQUIRE(items[i].value == ason_iter_long(iter));
					REQUIRE(! items[i].seen);
					items[i].seen = 1;
					free(str);
					break;
				}

				free(str);
			}

			REQUIRE(i < 3);
		} while(ason_iter_next(iter));

		for (i = 0; i < 3; i++)
			REQUIRE(items[i].seen);
	}

	ason_destroy(a);
	ason_iter_destroy(iter);

	a = ason_read("{ \"foo\": 6, \"bar\": 7, \"baz\": 8, * }");
	iter = ason_iterate(a);

	TEST("Universal Object Iteration") {
		struct {
			char name[4];
			int value;
			int seen;
		} items[3] = {
			{ "foo", 6, 0},
			{ "bar", 7, 0},
			{ "baz", 8, 0},
		};


		REQUIRE(ason_iter_type(iter) == ASON_TYPE_UOBJECT);
		REQUIRE(ason_iter_enter(iter));

		do {
			for (i = 0; i < 3; i++) {
				str = ason_iter_key(iter);

				if (! strcmp(items[i].name, str)) {
					REQUIRE(items[i].value == ason_iter_long(iter));
					REQUIRE(! items[i].seen);
					items[i].seen = 1;
					free(str);
					break;
				}

				free(str);
			}

			REQUIRE(i < 3);
		} while(ason_iter_next(iter));

		for (i = 0; i < 3; i++)
			REQUIRE(items[i].seen);
	}

	ason_destroy(a);
	ason_iter_destroy(iter);

	a = ason_read("[ \"foo\", \"bar\", \"baz\" ]");
	iter = ason_iterate(a);

	TEST("List Iteration") {
		char *values[3] = { "foo", "bar", "baz" };

		REQUIRE(ason_iter_type(iter) == ASON_TYPE_LIST);
		REQUIRE(ason_iter_enter(iter));

		i = 0;
		do {
			REQUIRE(i < 3);

			str = ason_iter_string(iter);

			REQUIRE(! strcmp(values[i], str));
			free(str);

			i++;
		} while(ason_iter_next(iter));
	}

	ason_destroy(a);
	ason_iter_destroy(iter);

	a = ason_read("[ \"foo\", [\"bar\", [\"baz\", 7.5], \"bang\"], \"bao\" ]");
	iter = ason_iterate(a);

	TEST("Nested List Iteration") {
		REQUIRE(ason_iter_type(iter) == ASON_TYPE_LIST);
		REQUIRE(ason_iter_enter(iter));

		str = ason_iter_string(iter);
		REQUIRE(! strcmp("foo", str));
		REQUIRE(ason_iter_next(iter));
		free(str);

		REQUIRE(ason_iter_type(iter) == ASON_TYPE_LIST);
		REQUIRE(ason_iter_next(iter));

		str = ason_iter_string(iter);
		REQUIRE(! strcmp("bao", str));
		REQUIRE(ason_iter_prev(iter));
		free(str);

		REQUIRE(ason_iter_type(iter) == ASON_TYPE_LIST);
		REQUIRE(ason_iter_enter(iter));

		str = ason_iter_string(iter);
		REQUIRE(! strcmp("bar", str));
		REQUIRE(ason_iter_next(iter));
		free(str);

		REQUIRE(ason_iter_type(iter) == ASON_TYPE_LIST);
		REQUIRE(ason_iter_enter(iter));

		str = ason_iter_string(iter);
		REQUIRE(! strcmp("baz", str));
		REQUIRE(ason_iter_next(iter));
		free(str);

		dbl = ason_iter_double(iter);
		REQUIRE(dbl == 7.5);
		REQUIRE(! ason_iter_next(iter));

		REQUIRE(ason_iter_exit(iter));
		REQUIRE(ason_iter_next(iter));

		str = ason_iter_string(iter);
		REQUIRE(! strcmp("bang", str));
		REQUIRE(! ason_iter_next(iter));
		free(str);

		REQUIRE(ason_iter_exit(iter));
		REQUIRE(ason_iter_next(iter));

		str = ason_iter_string(iter);
		REQUIRE(! strcmp("bao", str));
		REQUIRE(! ason_iter_next(iter));
		free(str);

		b = ason_iter_value(iter);
		c = ason_read("\"bao\"");
		REQUIRE(ason_check_equal(b, c));
		ason_destroy(b);
		ason_destroy(c);
	}

	ason_destroy(a);
	ason_iter_destroy(iter);

	a = ason_read("6");
	iter = ason_iterate(a);

	TEST("Iterate lonely singleton") {
		REQUIRE(! ason_iter_next(iter));
		REQUIRE(! ason_iter_prev(iter));
		REQUIRE(! ason_iter_enter(iter));
		REQUIRE(! ason_iter_exit(iter));
	}

	ason_destroy(a);
	ason_iter_destroy(iter);

	a = ason_read("6 | 7");
	iter = ason_iterate(a);

	TEST("Underrun") {
		REQUIRE(ason_iter_type(iter) == ASON_TYPE_UNION);
		REQUIRE(ason_iter_enter(iter));
		REQUIRE(! ason_iter_prev(iter));
		REQUIRE(ason_iter_next(iter));
		REQUIRE(ason_iter_prev(iter));
		REQUIRE(! ason_iter_prev(iter));
	}

	ason_destroy(a);
	ason_iter_destroy(iter);

	a = ason_read("[[[[[[[[[[6]]]]]]]]]]");
	b = ason_read("6");
	iter = ason_iterate(a);

	TEST("Deep iterate") {
		for (i = 0; i < 10; i++) {
			REQUIRE(ason_iter_type(iter) == ASON_TYPE_LIST);
			REQUIRE(ason_iter_enter(iter));
		}

		c = ason_iter_value(iter);
		REQUIRE(ason_check_equal(c, b));
	}

	ason_destroy(a);
	ason_destroy(b);
	ason_destroy(c);
	ason_iter_destroy(iter);

	return 0;
}

