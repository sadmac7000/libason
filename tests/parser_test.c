/**
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

/**
 * Basic exercise of the parser.
 **/
TEST_MAIN("Parse a simple value")
{
	ason_t *test_value = ason_read("{ \"foo\": 6, \"bar\": 8 } | 98 | [1,2,3] & [1,2]");
	ason_t *a;
	ason_t *b;
	ason_t *c;
	ason_t *d;

	a = ason_create_number(6);
	a = ason_create_object_d("foo", a);

	b = ason_create_number(8);
	b = ason_create_object_d("bar", b);

	a = ason_append_d(a, b);
	b = ason_create_number(98);

	a = ason_union_d(a, b);
	b = ason_create_number(1);
	b = ason_create_list_d(b);

	c = ason_create_number(2);
	c = ason_create_list_d(c);

	b = ason_append_d(b, c);
	c = ason_create_number(3);
	c = ason_create_list_d(c);

	d = ason_append(b, c);
	ason_destroy(c);

	b = ason_intersect_d(b, d);
	a = ason_union_d(a, b);
	
	REQUIRE(ason_check_corepresented(a, test_value));
	ason_destroy(test_value);
	ason_destroy(a);

	return 0;
}

