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
	ason_t *test_value = ason_read("{ \"foo\": 6 }");
	char *result = ason_asprint(test_value);
	
	printf("%s\n", result);
	ason_destroy(test_value);
	free(result);

	return 0;
}

