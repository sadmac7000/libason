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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "../src/blockfile.h"
#include "harness.h"

TESTS("Initialization",
      "Allocation",
      "False free",
      "Free",
      "Mapping",
      "Annotation",
      "Removing annotation",
      "Checking annotation",
      "Big regions");

/* TODO: Make autoconf set this for us */
#define TMPDIR "/tmp"

#define TMPFILE TMPDIR "/blockfile_test"

/**
 * Basic exercise of the parser.
 **/
TEST_MAIN("Blockfiles")
{
	TEST_INIT();
	blockfile_t *bf;
	int fd = -1;
	char buf[BLOCK_SIZE * 2];
	size_t to_read;
	ssize_t region;
	ssize_t region_b;
	void *mapping;
	void *mapping_b;
	void *raw_mapping;
	off_t seek;
	ssize_t got;
	unsigned char *loc = buf;
	size_t i;

	unlink(TMPFILE);

	TEST("Initialization") {
		bf = blockfile_open(TMPFILE);

		REQUIRE(bf);

		blockfile_close(bf);

		fd = open(TMPFILE, O_CLOEXEC | O_RDONLY);

		REQUIRE(fd >= 0);

		for (to_read = BLOCK_SIZE * 2; to_read; to_read -= got, loc += got) {
			got = read(fd, loc, BLOCK_SIZE * 2);

			REQUIRE(got != 0);

			if (got < 0 && errno != EAGAIN && errno != EINTR)
				err(1, "Read failed");

			if (got < 0)
				got = 0;
		}

		REQUIRE(! memcmp(BLOCK_MAGIC, buf, BLOCK_MAGIC_LENGTH));
		REQUIRE(! buf[BLOCK_MAGIC_LENGTH]);

		for (i = 0; i < BLOCK_SIZE; i++)
			REQUIRE(! buf[BLOCK_SIZE + i]);
	}

	unlink(TMPFILE);

	if (fd > 0)
		close(fd);

	fd = -1;

	bf = blockfile_open(TMPFILE);

	TEST("Allocation") {
		region = blockfile_allocate(bf, 10);

		REQUIRE(region >= 0);

		fd = open(TMPFILE, O_RDONLY | O_CLOEXEC);

		if (fd < 0)
			err(1, "Could not open temporary file");

		seek = lseek(fd, 0, SEEK_END);

		if (seek < 0)
			err(1, "Could noot seek");

		REQUIRE(! (seek % BLOCK_SIZE));
		REQUIRE(seek >= (12 * BLOCK_SIZE));
	}

	close(fd);
	blockfile_close(bf);
	unlink(TMPFILE);

	bf = blockfile_open(TMPFILE);
	region = blockfile_allocate(bf, 10);

	TEST("False free") {
		REQUIRE(blockfile_free(bf, region + 1) == -EINVAL);
		REQUIRE(blockfile_free(bf, region + 10) == -EINVAL);
	}

	TEST("Free") {
		REQUIRE(! blockfile_free(bf, region));
		REQUIRE(blockfile_free(bf, region) == -EINVAL);
	}

	blockfile_close(bf);
	unlink(TMPFILE);

	bf = blockfile_open(TMPFILE);
	region = blockfile_allocate(bf, 10);
	region_b = blockfile_allocate(bf, 1);
	mapping = mapping_b = raw_mapping = NULL;
	fd = -1;

	TEST("Mapping") {
		mapping = blockfile_map(bf, region);

		REQUIRE(mapping);

		mapping_b = blockfile_map(bf, region_b);

		REQUIRE(mapping_b);

		memset(mapping, 0xab, 10 * BLOCK_SIZE);
		memset(mapping_b, 0xcd, BLOCK_SIZE);

		/* Not sure how to verify blockfile_sync independently without
		 * depending on specific undefined kernel behavior.
		 */
		blockfile_sync(bf, mapping, 0, 10, 1);
		blockfile_sync(bf, mapping_b, 0, 1, 1);

		blockfile_unmap(bf, mapping);
		blockfile_unmap(bf, mapping_b);

		fd = open(TMPFILE, O_RDONLY | O_CLOEXEC);

		if (fd < 0)
			err(1, "Could not open temporary file");

		seek = lseek(fd, 0, SEEK_END);

		if (seek < 0)
			err(1, "Could noot seek");

		raw_mapping = mmap(NULL, seek, PROT_READ, MAP_PRIVATE, fd, 0);

		loc = raw_mapping;
		loc += (region + 2) * BLOCK_SIZE;

		for (i = 0; i < 10 * BLOCK_SIZE; i++)
			REQUIRE(loc[i] == 0xab);

		loc = raw_mapping + 2 * BLOCK_SIZE;
		loc += region_b * BLOCK_SIZE;

		for (i = 0; i < BLOCK_SIZE; i++)
			REQUIRE(loc[i] == 0xcd);
	}

	blockfile_close(bf);

	if (raw_mapping)
		munmap(raw_mapping, seek);

	if (fd >= 0)
		close(fd);

	unlink(TMPFILE);

	bf = blockfile_open(TMPFILE);
	fd = -1;
	raw_mapping = NULL;

	TEST("Annotation") {
		REQUIRE(! blockfile_annotate_block(bf, 10, "foo"));
		REQUIRE(! blockfile_annotate_block(bf, 10, "barr"));
		REQUIRE(! blockfile_annotate_block(bf, 15, "bazzz"));
		REQUIRE(! blockfile_annotate_block(bf, 20, "foo"));

		fd = open(TMPFILE, O_RDONLY | O_CLOEXEC);

		if (fd < 0)
			err(1, "Could not open temporary file");

		seek = lseek(fd, 0, SEEK_END);

		if (seek < 0)
			err(1, "Could noot seek");

		REQUIRE(seek == 2 * BLOCK_SIZE);

		raw_mapping = mmap(NULL, BLOCK_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);

		char test[] = "asonblok\0\0\0\0\0\0\0\3"
			"foo\0\0\0\0\0\0\0\x14\x4"
			"barr\0\0\0\0\0\0\0\xa\5bazzz\0\0\0\0\0\0\0\xf\0";

		REQUIRE(! memcmp(raw_mapping, test, sizeof(test) - 1));
	}

	if (raw_mapping)
		munmap(raw_mapping, BLOCK_SIZE);

	if (fd >= 0)
		close(fd);

	fd = -1;
	raw_mapping = NULL;

	TEST("Removing annotation") {
		blockfile_remove_annotation(bf, "barr");

		fd = open(TMPFILE, O_RDONLY | O_CLOEXEC);

		if (fd < 0)
			err(1, "Could not open temporary file");

		raw_mapping = mmap(NULL, BLOCK_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);

		char test[] = "asonblok\0\0\0\0\0\0\0\3"
			"foo\0\0\0\0\0\0\0\x14\x5"
			"bazzz\0\0\0\0\0\0\0\xf\0";

		REQUIRE(! memcmp(raw_mapping, test, sizeof(test) - 1));
	}

	if (raw_mapping)
		munmap(raw_mapping, BLOCK_SIZE);

	if (fd >= 0)
		close(fd);

	TEST("Checking annotation") {
		REQUIRE(blockfile_get_annotated_block(bf, "bar") == -ENOENT);
		REQUIRE(blockfile_get_annotated_block(bf, "bazzz") == 15);
	}

	blockfile_close(bf);
	unlink(TMPFILE);

	bf = blockfile_open(TMPFILE);

	region = blockfile_allocate(bf, BLOCK_COLOR_ENTRIES);
	region_b = blockfile_allocate(bf, 10);
	mapping = mapping_b = raw_mapping = NULL;
	fd = -1;

	TEST("Big regions") {
		mapping = blockfile_map(bf, region);

		REQUIRE(mapping);

		mapping_b = blockfile_map(bf, region_b);

		REQUIRE(mapping_b);

		memset(mapping, 0xab, BLOCK_COLOR_ENTRIES * BLOCK_SIZE);
		memset(mapping_b, 0xcd, 10 * BLOCK_SIZE);

		blockfile_sync(bf, mapping, 0, BLOCK_COLOR_ENTRIES , 1);
		blockfile_sync(bf, mapping_b, 0, 10, 1);

		blockfile_unmap(bf, mapping);
		blockfile_unmap(bf, mapping_b);

		fd = open(TMPFILE, O_RDONLY | O_CLOEXEC);

		if (fd < 0)
			err(1, "Could not open temporary file");

		seek = lseek(fd, 0, SEEK_END);

		if (seek < 0)
			err(1, "Could noot seek");

		raw_mapping = mmap(NULL, seek, PROT_READ, MAP_PRIVATE, fd, 0);

		loc = raw_mapping;
		loc += (region + 2) * BLOCK_SIZE;

		for (i = 0; i < BLOCK_COLOR_ENTRIES * BLOCK_SIZE; i++)
			REQUIRE(loc[i] == 0xab);

		loc = raw_mapping + 2 * BLOCK_SIZE;
		loc += region_b * BLOCK_SIZE;
		loc += BLOCK_SIZE; /* Additional colormap */

		for (i = 0; i < 10 * BLOCK_SIZE; i++)
			REQUIRE(loc[i] == 0xcd);
	}

	blockfile_close(bf);

	if (raw_mapping)
		munmap(raw_mapping, seek);

	if (fd >= 0)
		close(fd);

	unlink(TMPFILE);

	return 0;
}
