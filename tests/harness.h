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

#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <setjmp.h>
#include <string.h>
#include <err.h>

#include <ason/value.h>

typedef enum {
	TEST_SKIPPED = 0,
	TEST_PASSED,
	TEST_FAILED,
	TEST_PENDING,
} test_state_t;

struct test_info {
	test_state_t state[128];
	char test_name[128][41];
	size_t current;
	size_t count;
	void *abort_point;
};

extern struct test_info *test_info;

#define TESTS(count) \
	const size_t test_count = count; \

#define TEST_UPDATE_STATE() ({ \
	if (test_info->state[test_info->current] == TEST_PENDING) \
		test_info->state[test_info->current] = TEST_PASSED; })

#define JUMP_NAME_EXPAND(x) jump ## x
#define JUMP_NAME(x) JUMP_NAME_EXPAND(x)

#define TEST(_name) \
	test_info->state[test_info->current] = TEST_PENDING; \
	strcpy(test_info->test_name[test_info->current], _name); \
	JUMP_NAME(__LINE__): test_info->abort_point = &&JUMP_NAME(__LINE__); \
	for (; \
	     test_info->state[test_info->current] == TEST_PENDING; \
	     TEST_UPDATE_STATE(), \
	     test_info->current++)

#define TEST_MAIN(name) \
	const char *test_name = (name); \
	jmp_buf test_abort;	\
	int test_main(void)

#define EXIT_TEST_FAIL 88
#define REQUIRE(x) if (! (x)) { \
	printf("Failed: %s: %s:%d\n", #x, __FILE__, __LINE__); \
	fflush(stdout); \
	test_info->state[test_info->current] = TEST_FAILED; \
	test_info->current++; \
	goto *test_info->abort_point;  }

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif /* ASON_OUTPUT_H */
