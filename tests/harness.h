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

#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <ason/value.h>

typedef enum {
	TEST_PASSED,
	TEST_FAILED,
	TEST_PENDING,
	TEST_SKIPPED,
} test_state_t;

struct test_info {
	test_state_t state[2048];
	size_t current;
};

extern struct test_info *test_info;

#define TESTS(...) \
	const char *test_list[] = { __VA_ARGS__ }; \
	const size_t test_count = sizeof(test_list) / sizeof(char *)

#define TEST_INIT() ({ size_t i; test_info->current = 0; \
	for (i = 0; i < test_count; i++) \
		test_info->state[i] = TEST_SKIPPED; })

#define TEST(_ignore) \
	for (test_info->state[test_info->current] = TEST_PENDING; \
	     test_info->state[test_info->current] == TEST_PENDING; \
	     test_info->state[test_info->current++] = TEST_PASSED)

#define TEST_MAIN(name) \
	const char *test_name = (name); \
	int test_main(void)

#define EXIT_TEST_FAIL 88
#define REQUIRE(x) if (! (x)) { \
	test_info->state[test_info->current] = TEST_FAILED; \
	break; }

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif /* ASON_OUTPUT_H */
