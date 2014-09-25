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

TESTS(13);

/**
 * Basic exercise of namespaces.
 **/
TEST_MAIN("Namespaces")
{
	ason_t *a;
	ason_t *b = NULL;
	ason_t *c = NULL;
	ason_t *d = NULL;
	ason_ns_t *root;
	ason_ns_t *sub_1 = NULL;
	ason_ns_t *sub_2 = NULL;

	a = ason_read("{ \"foo\": 6, \"bar\": 7, \"baz\": 8 }", NULL);

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

	b = ason_read("\"stringval\"", NULL);
	TEST("Clobbering value") {
		REQUIRE(! ason_ns_store(root, "a", b));
		d = ason_ns_load(root, "a");
		REQUIRE(ason_check_equal(b, d));
	}

	ason_destroy(b);
	ason_destroy(d);
	d = NULL;

	ason_ns_destroy(root);

	root = ason_ns_create(ASON_NS_RAM, NULL);
	sub_1 = ason_ns_create(ASON_NS_RAM, NULL);

	TEST("Subspace attachment") {
		REQUIRE(! ason_ns_get_sub(root, "sub_1"));
		REQUIRE(ason_ns_attach(sub_1, root, "sub_1") == sub_1);
		REQUIRE(ason_ns_get_sub(root, "sub_1") == sub_1);
	}

	sub_2 = ason_ns_create(ASON_NS_RAM, NULL);
	ason_ns_attach(sub_2, root, "sub_2");

	TEST("Subspace storage") {
		REQUIRE(! ason_ns_load(root, "a"));
		REQUIRE(! ason_ns_load(sub_1, "a"));
		REQUIRE(! ason_ns_load(root, "sub_1.a"));
		REQUIRE(! ason_ns_load(sub_2, "a"));
		REQUIRE(! ason_ns_load(root, "sub_2.a"));

		REQUIRE(ason_ns_store(root, "sub_1.a", a) == -ENOENT);
		REQUIRE(! ason_ns_mkvar(root, "sub_1.a"));
		REQUIRE(! ason_ns_store(root, "sub_1.a", a));

		REQUIRE(! ason_ns_load(root, "a"));
		REQUIRE(! ason_ns_load(sub_2, "a"));
		REQUIRE(! ason_ns_load(root, "sub_2.a"));

		c = ason_ns_load(sub_1, "a");
		d = ason_ns_load(root, "sub_1.a");

		REQUIRE(ason_check_equal(a, d));
		REQUIRE(ason_check_equal(c, d));
	}

	ason_destroy(a);
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
	a = ason_read("6", NULL);

	TEST("Namespace by URL") {
		root = ason_ns_connect("ram");

		REQUIRE(root);

		REQUIRE(! ason_ns_mkvar(root, "a"));
		REQUIRE(! ason_ns_store(root, "a", a));
		d = ason_ns_load(root, "a");
		REQUIRE(ason_check_equal(a, d));

		ason_destroy(d);
		ason_ns_destroy(root);
		/* Retry post-init */

		root = ason_ns_connect("ram:ignored");

		REQUIRE(root);

		REQUIRE(! ason_ns_mkvar(root, "a"));
		REQUIRE(! ason_ns_store(root, "a", a));
		d = ason_ns_load(root, "a");
		REQUIRE(ason_check_equal(a, d));
	}

	ason_ns_destroy(root);
	ason_destroy(a);
	ason_destroy(d);

	TEST("Bad namespace URL") {
		REQUIRE(! ason_ns_connect("bullshit"));
	}

	root = ason_ns_create(ASON_NS_RAM, NULL);

	TEST("Bad subspaces") {
		REQUIRE(! ason_ns_get_sub(root, "bullshit"));
		REQUIRE(! ason_ns_get_sub(root, "bullshit.subspace"));
		REQUIRE(! ason_ns_load(root, "bullshit.var"));
		REQUIRE(ason_ns_store(root, "bullshit.var", ASON_NULL) ==
			-ENOENT);
		REQUIRE(ason_ns_mkvar(root, "bullshit.var") == -ENOENT);
		REQUIRE(ason_ns_set_meta(root, "bullshit.var", "") == -ENOENT);
		REQUIRE(! ason_ns_get_meta(root, "bullshit.var"));
	}

	ason_ns_destroy(root);

	root = ason_ns_create(ASON_NS_RAM, NULL);
	sub_1 = ason_ns_create(ASON_NS_RAM, NULL);
	sub_2 = ason_ns_create(ASON_NS_RAM, NULL);

	TEST("Bad attachment points") {
		REQUIRE(! ason_ns_attach(sub_1, root, ""));
		REQUIRE(! ason_ns_attach(sub_1, root, "!good"));
	}

	ason_ns_attach(sub_1, root, "sub_1");
	a = ason_read("6", NULL);

	TEST("Move namespaces") {
		REQUIRE(! ason_ns_mkvar(root, "sub_1.a"));
		REQUIRE(! ason_ns_store(root, "sub_1.a", a));
		d = ason_ns_load(root, "sub_1.a");
		REQUIRE(ason_check_equal(a, d));
		ason_destroy(d);

		REQUIRE(ason_ns_attach(sub_1, sub_2, "sub_1a"));

		d = ason_ns_load(sub_2, "sub_1a.a");
		REQUIRE(ason_check_equal(a, d));
		ason_destroy(d);
	}

	ason_destroy(a);
	ason_ns_destroy(root);
	ason_ns_destroy(sub_2);

	TEST("Register bad protocol name") {
		REQUIRE(ason_ns_register_proto(NULL, "") == -EINVAL);
	}

	return 0;
}

