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

#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <setjmp.h>

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
	size_t to_go;
};

extern struct test_info *test_info;

#define TESTS(...) \
	const char *test_list[] = { __VA_ARGS__ }; \
	const size_t test_count = sizeof(test_list) / sizeof(char *)

#define TEST_INIT() ({ size_t i; test_info->to_go = test_count; \
	for (i = 0; i < test_count; i++) \
		test_info->state[i] = TEST_SKIPPED; })

#define TEST_LOOKUP_NAME(_name) ({					\
	const char *name = (_name);					\
	size_t i;							\
	for (i = 0; i < test_count && strcmp(test_list[i], name); i++); \
	if (i == test_count)						\
		errx(1, "No such test: %s", name);			\
	i;								\
})

#define TEST(_name) \
	test_info->to_go--; \
	test_info->current = TEST_LOOKUP_NAME(_name); \
	test_info->state[test_info->current] = TEST_PENDING; \
	setjmp(test_abort); \
	for (; \
	     test_info->state[test_info->current] == TEST_PENDING; \
	     test_info->state[test_info->current] = TEST_PASSED)

#define TEST_MAIN(name) \
	const char *test_name = (name); \
	jmp_buf test_abort;	\
	int test_main(void)

#define EXIT_TEST_FAIL 88
#define REQUIRE(x) if (! (x)) { \
	printf("Failed: %s: %s:%d\n", #x, __FILE__, __LINE__); \
	test_info->state[test_info->current] = TEST_FAILED; \
	longjmp(test_abort, 1); }

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif /* ASON_OUTPUT_H */
