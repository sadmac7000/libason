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
#include <ctype.h>
#include <string.h>

#include <ason/value.h>
#include <ason/output.h>
#include <ason/read.h>

#include "harness.h"

TESTS(21);

static void
strip_spaces(char *str)
{
	size_t i = 0;
	size_t j = 0;

	for (i = 0; str[i]; i++)
		if (! isspace(str[i]))
			str[j++] = str[i];

	str[j] = '\0';
}

/**
 * Test object printing methods.
 **/
TEST_MAIN("Object printing")
{
	ason_t *test = NULL;
	char *output = NULL;

#define TEST_OUTPUT(_name, _str) \
	TEST(_name) { \
		test = ason_read(_str, NULL); \
		output = ason_asprint(test); \
		strip_spaces(output); \
		REQUIRE(!strcmp(_str, output)); \
	} \
	free(output); \
	ason_destroy(test)

#define TEST_OUTPUT_U(_name, _str) \
	TEST(_name) { \
		test = ason_read(_str, NULL); \
		output = ason_asprint_unicode(test); \
		strip_spaces(output); \
		REQUIRE(!strcmp(_str, output)); \
	} \
	free(output); \
	ason_destroy(test)


	TEST_OUTPUT("Integer", "6");
	TEST_OUTPUT("String", "\"foo\"");
	TEST_OUTPUT("Null", "null");
	TEST_OUTPUT("Universe", "U");
	TEST_OUTPUT("Wild", "*");
	TEST_OUTPUT("Empty", "_");
	TEST_OUTPUT_U("Empty (Unicode)", "∅");
	TEST_OUTPUT("True", "true");
	TEST_OUTPUT("False", "false");
	TEST_OUTPUT("Object", "{\"bar\":6,\"foo\":7}");
	TEST_OUTPUT("Universal Object", "{\"bar\":6,\"foo\":7,*}");
	TEST_OUTPUT("List", "[6,7,8]");
	TEST_OUTPUT("Union", "6|7");
	TEST_OUTPUT_U("Union (Unicode)", "6∪7");
	TEST_OUTPUT("Empty Universal Object", "{*}");
	TEST_OUTPUT("Empty Object", "{}");
	TEST_OUTPUT("Complement", "!6");
	TEST_OUTPUT("Union Complement", "!(6|7)");
	TEST_OUTPUT("Escapes", "\"\\\"\\\\\\/\\b\\f\\n\\r\\t\\v\"");

	TEST("Control character escaping") {
		test = ason_read("\"\b\"", NULL);
		output = ason_asprint_unicode(test);
		REQUIRE(!strcmp("\"\\b\"", output));
	}

	TEST("Unicode escaping") {
		test = ason_read("\"©\\u00A9\"", NULL);
		output = ason_asprint_unicode(test);
		REQUIRE(!strcmp("\"\\u00a9\\u00a9\"", output));
	}

	free(output);
	ason_destroy(test);

	return 0;
}
