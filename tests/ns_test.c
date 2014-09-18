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
#include <ason/output.h>
#include <ason/read.h>
#include <ason/namespace.h>

#include "harness.h"

TESTS(8);

/**
 * Basic exercise of namespaces.
 **/
TEST_MAIN("Namespaces")
{
	ason_t *a;
	ason_t *b;
	ason_t *c;
	ason_t *d = NULL;
	ason_ns_t *root;
	ason_ns_t *sub1 = NULL;
	ason_ns_t *sub2 = NULL;

	a = ason_read("{ \"foo\": 6, \"bar\": 7, \"baz\": 8 }", NULL);
	b = ason_read("\"stringval\"", NULL);
	c = ason_read("6", NULL);

	root = ason_ns_create(ASON_NS_RAM, NULL);

	TEST("Value storage") {
		REQUIRE(! ason_ns_mkvar(root, "a"));
		REQUIRE(! ason_ns_store(root, "a", a));
		d = ason_ns_load(root, "a");
		REQUIRE(ason_check_equal(a, d));
	}

	ason_destroy(d);
	d = NULL;

	TEST("Loading of non-existent value") {
		REQUIRE(! ason_ns_load(root, "nothing_here"));
	}

	TEST("Storing to non-existent value") {
		REQUIRE(ason_ns_store(root, "nothing_here", b) == -ENOENT);
		REQUIRE(! ason_ns_load(root, "nothing_here"));
	}

	TEST("Clobbering value") {
		REQUIRE(! ason_ns_store(root, "a", b));
		d = ason_ns_load(root, "a");
		REQUIRE(ason_check_equal(b, d));
	}

	ason_destroy(d);
	d = NULL;

	ason_ns_destroy(root);

	root = ason_ns_create(ASON_NS_RAM, NULL);
	sub1 = ason_ns_create(ASON_NS_RAM, NULL);

	TEST("Subspace attachment") {
		REQUIRE(! ason_ns_get_sub(root, "sub1"));
		REQUIRE(ason_ns_attach(sub1, root, "sub1") == sub1);
		REQUIRE(ason_ns_get_sub(root, "sub1") == sub1);
	}

	sub2 = ason_ns_create(ASON_NS_RAM, NULL);
	ason_ns_attach(sub2, root, "sub2");

	TEST("Subspace storage") {
		REQUIRE(! ason_ns_load(root, "a"));
		REQUIRE(! ason_ns_load(sub1, "a"));
		REQUIRE(! ason_ns_load(root, "sub1.a"));
		REQUIRE(! ason_ns_load(sub2, "a"));
		REQUIRE(! ason_ns_load(root, "sub2.a"));

		REQUIRE(ason_ns_store(root, "sub1.a", a) == -ENOENT);
		REQUIRE(! ason_ns_mkvar(root, "sub1.a"));
		REQUIRE(! ason_ns_store(root, "sub1.a", a));

		REQUIRE(! ason_ns_load(root, "a"));
		REQUIRE(! ason_ns_load(sub2, "a"));
		REQUIRE(! ason_ns_load(root, "sub2.a"));

		c = ason_ns_load(sub1, "a");
		d = ason_ns_load(root, "sub1.a");

		REQUIRE(ason_check_equal(a, d));
		REQUIRE(ason_check_equal(c, d));
	}

	ason_destroy(a);
	ason_destroy(b);
	ason_destroy(c);
	ason_destroy(d);

	ason_ns_destroy(root);

	root = ason_ns_create(ASON_NS_RAM, NULL);

	TEST("Variable safety") {
		REQUIRE(ason_ns_mkvar(root, "") == -EINVAL);
		REQUIRE(ason_ns_mkvar(root, "3mium") == -EINVAL);
		REQUIRE(ason_ns_mkvar(root, "v@riable") == -EINVAL);
		REQUIRE(ason_ns_mkvar(root, "v@riable") == -EINVAL);
		REQUIRE(ason_ns_mkvar(root, "value_3") == 0);
		REQUIRE(ason_ns_mkvar(root, "value_4") == 0);
		REQUIRE(ason_ns_mkvar(root, "value_3") == -EEXIST);
	}

	TEST("Unsupported calls") {
		char *random_pointer = "POISONNN";

		REQUIRE(ason_ns_set_meta(root, "value_2", random_pointer) == -ENOTSUP);
		REQUIRE(ason_ns_set_meta(root, "value_3", random_pointer) == -ENOTSUP);
		REQUIRE(*ason_ns_get_meta(root, "value_2") == '\0');
		REQUIRE(*ason_ns_get_meta(root, "value_3") == '\0');
	}

	ason_ns_destroy(root);

	return 0;
}

