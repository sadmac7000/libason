/**
 * This file is part of libasonalg.
 *
 * libasonalg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libasonalg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libasonalg. If not, see <http://www.gnu.org/licenses/>.
 **/

#include <ason/value.h>
#include <ason/output.h>

const char *test_name = "Object printing";

/**
 * A test which does nothing.
 **/
int
test_main(void)
{
	ason_t *six = ason_create_number(6);
	ason_t *seven = ason_create_number(7);
	ason_t *eight = ason_create_number(8);
	ason_t *list = ason_create_list(six);
	ason_t *object = ason_create_object("first", six);
	ason_t *a, *b;
	char *out;

	a = ason_create_list(seven);
	b = ason_append(list, a);
	ason_destroy(list);
	ason_destroy(a);
	list = b;

	a = ason_create_list(eight);
	b = ason_append(list, a);
	ason_destroy(list);
	ason_destroy(a);
	list = b;

	a = ason_create_object("second", seven);
	b = ason_append(object, a);
	ason_destroy(object);
	ason_destroy(a);
	object = b;

	a = ason_create_object("third", eight);
	b = ason_append(object, a);
	ason_destroy(object);
	ason_destroy(a);
	object = b;

	a = ason_create_object("all", list);
	b = ason_append(object, a);
	ason_destroy(object);
	ason_destroy(a);
	ason_destroy(list);
	object = b;

	out = ason_asprint(object);
	printf("%s\n", out);
	out = ason_asprint_unicode(object);
	printf("%s\n", out);

	a = ason_flatten(object);
	ason_destroy(a);
	object = a;

	out = ason_asprint(object);
	printf("%s\n", out);
	out = ason_asprint_unicode(object);
	printf("%s\n", out);

	return 0;
}
