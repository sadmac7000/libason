/**
 * Copyright © 2013, 2014 Red Hat, Casey Dahlin <cdahlin@redhat.com>
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

#include <string.h>
#include <endian.h>

#include "crc.h"

/* CRC-64-ECMA polynomial, see ECMA-182 */
#define POLY 0x42F0E1EBA9EA3693 

/**
 * Calculate the crc-64 checksum for 64 bits of data which have been packed
 * into a 64-bit unsigned integer in host byte-order.
 **/
uint64_t
crc64_8h(uint64_t data)
{
	uint64_t remainder_xor = POLY << 63;
	uint64_t test_bit = (uint64_t)1 << 63;
	uint64_t xor = (POLY >> 1) | test_bit;
	uint64_t remainder = 0;

	while (test_bit) {
		if (data & test_bit) {
			data ^= xor;
			remainder ^= remainder_xor;
		}

		test_bit >>= 1;
		xor >>= 1;
		remainder_xor = test_bit * POLY;
	}

	return remainder;
}

/**
 * Calculate the CRC-64 checksum for a stream of data. Return as a host
 * byte-order integer.
 **/
uint64_t
crc64(unsigned char *data, size_t size)
{
	uint64_t in;
	uint64_t out = 0;

	while (size >= 8) {
		memcpy(&in, data, 8);
		in = be64toh(in);

		out = crc64_8h(in ^ out);
		size -= 8;
		data += 8;
	}

	if (! size)
		return out;

	in = 0;

	memcpy(&in, data, size);
	in = be64toh(in);
	in ^= out;

	in >>= 64 - (size * 8);
	out <<= size * 8;
	out ^= crc64_8h(in);

	return out;
}
