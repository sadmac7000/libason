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
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <err.h>

#include <ason/ason.h>
#include <ason/print.h>

#include "value.h"
#include "num_domain.h"
#include "util.h"
#include "stringfunc.h"

/**
 * Print an ASON value as a string. Flag indicates if unicode should be used.
 **/
static char *
ason_do_asprint(ason_t *value, int unicode)
{
	size_t i;
	char *tmp;
	char *ret = NULL;
	char *prefix;
	char *sep;
	char *oper;
	ason_num_dom_t *dom = value->num_dom;
	int state;
	int elem_state;

	if (value->num_dom == NULL)
		return xstrdup(unicode ? "∅" : "_");
	if (value->num_dom == ASON_NUM_DOM_UNIVERSE)
		return xstrdup("NUMBERS");

	state = dom->minus_inf;

	for (i = 0; i < dom->count; i++) {
		tmp = ret;
		prefix = ret ?: "";
		elem_state = TWOBIT_GET(dom->states, i) ^ dom->inv_bits;

		if (! ret)
			sep = "";
		else if (state)
			sep = unicode ? "∩" : "&";
		else
			sep = unicode ? " ∪ " : " | ";

		if (elem_state % 3)
			oper = state ? "!" : "";
		else if (elem_state)
			oper = state ? ">=" : "<=";
		else
			oper = state ? ">" : "<";

		if ((elem_state % 3) == 0)
			state = !state;

		ret = xasprintf("%s%s%s%d", prefix, sep, oper,
				FP_WHOLE(dom->items[i]));
		free(tmp);
	}

	if (! ret)
		errx(1, "ason_asprint did not generate a string");
	return ret;
}

/**
 * Print an ASON value as an ASCII string.
 **/
API_EXPORT char *
ason_asprint(ason_t *value)
{
	return ason_do_asprint(value, 0);
}

/**
 * Print an ASON value as a unicode string.
 **/
API_EXPORT char *
ason_asprint_unicode(ason_t *value)
{
	return ason_do_asprint(value, 1);
}
