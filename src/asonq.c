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
#include <err.h>
#include <string.h>
#include <string.h>
#include <wordexp.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <ason/read.h>
#include <ason/namespace.h>
#include <ason/ason.h>
#include <ason/print.h>

/**
 * Connect a new ASON namespace.
 **/
static void
connect(const char *line, ason_ns_t *ns)
{
	ason_ns_t *sub;
	char *name;
	const char *pos;

	while (isspace(*line))
		line++;

	pos = line;

	while (isalpha(*pos) || isdigit(*pos) || *pos == '_')
		pos++;

	if (*pos && !isspace(*pos)) {
		printf("Invalid namespace name\n");
		return;
	}

	name = strndup(line, pos - line);

	if (! name)
		errx(1, "Out of Memory");

	while (isspace(*pos))
		pos++;

	sub = ason_ns_connect(pos);

	if (! sub) {
		printf("Could not connect\n");
		return;
	}

	if (! ason_ns_attach(sub, ns, name)) {
		printf("Namespace already exists");
		ason_ns_destroy(sub);
	}

	free(name);
}

/**
 * Process a non-ASON command.
 **/
static int
command(const char *line, ason_ns_t *ns)
{
	if (*(line++) != '/')
		return 0;

	if (! strcmp(line, "quit"))
		exit(0);
	else if (!strncmp(line, "connect", 7))
		connect(line + 7, ns);
	else
		printf("Command not recognized\n");

	return 1;
}

/**
 * A simple ASON REPL shell.
 **/
int
main(void)
{
	ason_ns_t *ns = ason_ns_create(ASON_NS_RAM, "");
	char *line;
	ason_t *value;
	wordexp_t exp;

	if (wordexp("~/.asonq", &exp, 0))
		errx(1, "Could not perform shell expansion");

	if (! ns)
		errx(1, "Could not create namespace");

	using_history();
	read_history(exp.we_wordv[0]);

	for (;;) {
		line = readline("> ");

		if (! line) {
			printf("\n");
			break;
		}

		if (! *line) {
			free(line);
			continue;
		}

		add_history(line);
		write_history(exp.we_wordv[0]);

		if (command(line, ns)) {
			free(line);
			continue;
		}

		value = ason_read(line, ns);

		free(line);

		if (! value) {
			printf("Syntax Error\n");
			continue;
		}

		line = ason_asprint_unicode(value);

		if (! line)
			errx(1, "Could not serialize value");

		printf("%s\n", line);
		free(line);
	}
}
