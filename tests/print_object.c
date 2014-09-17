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
 * A test which does nothing.
 **/
TEST_MAIN("Object printing")
{
	TEST_INIT();
	ason_t *six = ason_create_number(6);
	ason_t *seven = ason_create_number(7);
	ason_t *eight = ason_create_number(8);
	ason_t *list = ason_create_list(six);
	ason_t *object = ason_create_object("first", six);
	ason_t *test;
	ason_t *a, *b;
	char *out;

	a = ason_create_list(seven);
	b = ason_join(list, a);
	ason_destroy(list);
	ason_destroy(a);
	list = b;

	a = ason_create_list(eight);
	b = ason_join(list, a);
	ason_destroy(list);
	ason_destroy(a);
	list = b;

	a = ason_create_object("second", seven);
	b = ason_join(object, a);
	ason_destroy(object);
	ason_destroy(a);
	object = b;

	a = ason_create_object("third", eight);
	b = ason_join(object, a);
	ason_destroy(object);
	ason_destroy(a);
	object = b;

	a = ason_create_object("all", list);
	b = ason_join(object, a);
	ason_destroy(object);
	ason_destroy(a);
	ason_destroy(list);
	object = b;

	TEST("ASCII printing") {
		out = ason_asprint(object);
		test = ason_read(out, NULL);

		REQUIRE(test);
		REQUIRE(ason_check_equal(object, test));
	}

	free(out);
	ason_destroy(test);

	TEST("Unicode printing") {
		out = ason_asprint_unicode(object);
		test = ason_read(out, NULL);

		REQUIRE(test);
		REQUIRE(ason_check_equal(object, test));
	}

	free(out);
	ason_destroy(test);

	ason_destroy(object);
	ason_destroy(six);
	ason_destroy(seven);
	ason_destroy(eight);

	return 0;
}
