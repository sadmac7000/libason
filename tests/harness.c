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
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include "harness.h"

/**
 * Special return code for when the harness itself breaks.
 **/
#define EXIT_HARNESS_FAIL 122

/**
 * Shared memory global for test state.
 **/
struct test_info *test_info;

/**
 * Globals where we store the test parameters.
 **/
extern const char *test_name;
extern const size_t test_count;
extern const char *test_list[];

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Man body of the test.
 **/
int test_main(void);

#ifdef __cplusplus
}
#endif

/**
 * Autotools-mandated options.
 **/
struct option options[] = {
	{ "test-name", required_argument, NULL, 0, },
	{ "log-file", required_argument, NULL, 0, },
	{ "trs-file", required_argument, NULL, 0, },
	{ "color-tests", required_argument, NULL, 0, },
	{ "expect-failure", required_argument, NULL, 0, },
	{ "enable-hard-errors", required_argument, NULL, 0, },
	{ "raw", 0, NULL, 0, },
};

/**
 * Return test results.
 **/
void
record_test(const char *name, FILE *trs_fp, int code,
	    int status, test_state_t state, int xfail)
{
	if (state == TEST_PASSED) {
		fprintf(trs_fp, ":test-result: %sPASS %s\n",
			xfail ? "X" : "", name);
	} else if (state == TEST_FAILED) {
		fprintf(trs_fp, ":test-result: %sFAIL %s\n",
			xfail ? "X" : "", name);
	} else if (state == TEST_SKIPPED) {
		fprintf(trs_fp, ":test-result: SKIP %s\n", name);
	} else if (code != CLD_EXITED) {
		fprintf(trs_fp, ":test-result: ERROR %s (signal %d)\n",
			name, status);
	} else {
		fprintf(trs_fp, ":test-result: ERROR %s (return code %d)\n",
			name, status);
	}
}

void
cleanup_mapping(void)
{
	munmap(test_info, sizeof(struct test_info));
}

/**
 * Main unit tester.
 **/
int
main(int argc, char **argv)
{
	pid_t pid;
	siginfo_t info;
	int ret;
	int logfile = 1;
	int trsfile = 1;
	char test_name_field[] = "........................................";
	int idx;
	FILE *trs_fp = stdout;
	int xfail = 0;
#if 0 /* unused */
	int color = 0;
	int enable_hard = 1;
#endif
	int pass = 1;
	size_t i;

	if (strlen(test_name) > 40)
		errx(1, "FATAL: Really long test name in %s", argv[0]);

	test_info = mmap(NULL, sizeof(struct test_info), PROT_READ | PROT_WRITE,
			 MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	if (! test_info)
		err(1, "FATAL: Could not map shared segment");

	atexit(cleanup_mapping);

	while (getopt_long(argc, argv, "+", options, &idx) >= 0) {
		if (idx == 1) {
			logfile = open(optarg, O_WRONLY | O_CREAT | O_TRUNC, 0600);

			if (logfile < 0)
				err(1, "FATAL: Could not open log %s", optarg);
		}

		if (idx == 2) {
			trsfile = open(optarg, O_WRONLY | O_CREAT | O_TRUNC, 0600);

			if (trsfile < 0)
				err(1, "FATAL: Could not open TRS %s", optarg);

			trs_fp = fdopen(trsfile, "w");

			if (! trs_fp)
				err(1, "FATAL: Could not create buffer for TRS %s", optarg);
		}

#if 0 /* unused */
		if (idx == 3 && !strcmp("yes", optarg))
			color = 1;
#endif
		if (idx == 4 && !strcmp("yes", optarg))
			xfail = 1;
#if 0 /* unused */
		if (idx == 5 && !strcmp("no", optarg))
			enable_hard = 0;
#endif

		if (idx == 6)
			exit(test_main());
	}

	memcpy(test_name_field, test_name, strlen(test_name));

	close(0);
	if (open("/dev/null", O_RDONLY))
		err(1, "FATAL: Could not open /dev/null");

	do {
		ret = dup2(logfile, 2);
	} while (ret < 0 && errno == EINTR);

	/* Will this error message make it? O_O */
	if (ret < 0)
		err(EXIT_HARNESS_FAIL,
		    "FATAL: Could not set stderr");

	pid = fork();

	if (pid < 0)
		err(1, "FATAL: Could not fork");

	if (! pid) {
		do {
			ret = dup2(logfile, 1);
		} while (ret < 0 && errno == EINTR);

		if (ret < 0)
			err(EXIT_HARNESS_FAIL,
			    "FATAL: Could not set stdout");

		exit(test_main());
	}

	do {
		ret = waitid(P_PID, pid, &info, WEXITED);
	} while (ret && errno == EINTR);

	if (ret)
		err(1, "FATAL: Could not gather child");

	for (i = 0; i < test_count; i++) {
		if (test_info->state[i] != TEST_PASSED)
			pass = 0;

		record_test(test_list[i], trs_fp, info.si_code,
			    info.si_status, test_info->state[i],
			    xfail);
	}

	if (! test_info->to_go) {
		if (pass) {
			fprintf(trs_fp, ":test-global-result: PASS %s\n", test_name);
			printf("TEST: %sPASS\n", test_name_field);
		} else {
			fprintf(trs_fp, ":test-global-result: FAIL %s\n", test_name);
			printf("TEST: %sFAIL\n", test_name_field);
		}
	} else if (test_info->state[test_info->current] != TEST_PENDING) {
		fprintf(trs_fp, ":test-global-result: SETUP-ERROR %s\n", test_name);
		printf("TEST: %sSETUP-ERROR\n", test_name_field);
	} else {
		fprintf(trs_fp, ":test-global-result: ERROR %s\n", test_name);
		printf("TEST: %sERROR\n", test_name_field);
	}

	return 0;
}
