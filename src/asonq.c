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
#include <err.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <ason/read.h>
#include <ason/namespace.h>
#include <ason/value.h>
#include <ason/output.h>

/**
 * Process a non-ASON command.
 **/
static int
command(const char *line)
{
	if (*(line++) != '/')
		return 0;

	if (! strcmp(line, "quit"))
		exit(0);
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

	if (! ns)
		errx(1, "Could not create namespace");

	for (;;) {
		line = readline("> ");

		if (! (line && *line)) {
			free(line);
			continue;
		}

		add_history(line);

		if (command(line)) {
			free(line);
			continue;
		}

		value = ason_read(line, ns);

		free(line);

		if (! value) {
			printf("Syntax Error\n");
			continue;
		}

		line = ason_asprint(value);

		if (! line)
			errx(1, "Could not serialize value");

		printf("%s\n", line);
		free(line);
	}
}
