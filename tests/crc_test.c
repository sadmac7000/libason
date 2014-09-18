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
#include <endian.h>

#include "../src/crc.h"
#include "harness.h"

TESTS(2);

/**
 * Basic exercise of the parser.
 **/
TEST_MAIN("CRC")
{
	TEST("Single-word CRC") {
		REQUIRE(crc64_8h(0x123456789abcdef0) == 0x13e76db181b0d129);
	}

	TEST("Irregular CRC") {
		unsigned char buf[21] =
			"Hello, world!\x1f\x73\xc2\xab\xcf\x65\x43\x1e";

		REQUIRE(crc64(buf, 21) == 0);
	}

	return 0;
}

