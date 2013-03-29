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
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <err.h>

#include <ason/value.h>
#include <ason/output.h>

#include "value.h"
#include "util.h"

/* Predeclaration */
static char *ason_do_asprint(ason_t *value, int use_unicode);

/**
 * Get a precedence value for an operator.
 **/
static int
ason_get_precedence(ason_type_t operator)
{
	switch (operator) {
	case ASON_COQUERY:
		return 0;
	case ASON_INTERSECT:
		return 1;
	case ASON_QUERY:
		return 2;
	case ASON_APPEND:
		return 3;
	default:
		return INT_MAX;
	}
}

/**
 * Get the character for an ASON operator.
 **/
static const char *
ason_get_opchar(ason_type_t operator, int use_unicode)
{
	switch (operator) {
	case ASON_COQUERY:
		if (use_unicode)
			return "⋈";
		else
			return "~~";
	case ASON_INTERSECT:
		if (use_unicode)
			return "∩";
		else
			return "&";
	case ASON_QUERY:
		if (use_unicode)
			return "⊳";
		else
			return "~";
	case ASON_APPEND:
		return ".";
	default:
		errx(1, "Unreachable statement at %s:%d", __FILE__, __LINE__);
	}
}

/**
 * Print an ASON operator as a string.
 **/
static char *
ason_asprint_operator(ason_t *value, int use_unicode)
{
	char *a = ason_do_asprint(value->items[0], use_unicode);
	char *b = ason_do_asprint(value->items[1], use_unicode);
	const char *op = ason_get_opchar(value->type, use_unicode);
	char *tmp;
	int precedence = ason_get_precedence(value->type);

	if (precedence > ason_get_precedence(value->items[0]->type)) {
		tmp = a;
		a = xasprintf("( %s )", a);
		free(tmp);
	}

	if (precedence > ason_get_precedence(value->items[1]->type)) {
		tmp = b;
		b = xasprintf("( %s )", b);
		free(tmp);
	}

	tmp = xasprintf("%s %s %s", a, op, b);
	free(a);
	free(b);

	return tmp;
}

/**
 * Print an ASON union as a string.
 **/
static char *
ason_asprint_union(ason_t *value, int use_unicode)
{
	char *out = NULL;
	char *tmp;
	char *next;
	char *sep = use_unicode ? "∪" : "|";
	size_t i;

	for (i = 0; i < value->count; i++) {
		next = ason_do_asprint(value->items[i], use_unicode);

		if (out) {
			tmp = out;
			out = xasprintf("%s %s %s", out, sep, next);
			free(tmp);
			free(next);
		} else {
			out = next;
		}
	}

	return out;
}

/**
 * Print an ASON object as a string.
 **/
static char *
ason_asprint_object(ason_t *value, int use_unicode)
{
	char *out = NULL;
	char *tmp;
	char *next;
	size_t i;

	/* FIXME: Escaping for keys. */
	for (i = 0; i < value->count; i++) {
		next = ason_do_asprint(value->kvs[i].value, use_unicode);

		tmp = next;
		next = xasprintf("\"%s\": %s", value->kvs[i].key, next);
		free(tmp);

		if (out) {
			tmp = out;
			out = xasprintf("%s, %s", out, next);
			free(tmp);
			free(next);
		} else {
			out = next;
		}
	}

	if (value->type == ASON_UOBJECT)
		tmp = xasprintf("{ %s, *}", out);
	else
		tmp = xasprintf("{ %s }", out);
	free(out);
	return tmp;
}

/**
 * Print an ASON list as a string.
 **/
static char *
ason_asprint_list(ason_t *value, int use_unicode)
{
	char *out = NULL;
	char *tmp;
	char *next;
	size_t i;

	for (i = 0; i < value->count; i++) {
		next = ason_do_asprint(value->items[i], use_unicode);

		if (out) {
			tmp = out;
			out = xasprintf("%s, %s", out, next);
			free(tmp);
			free(next);
		} else {
			out = next;
		}
	}

	tmp = xasprintf("[ %s ]", out);
	free(out);
	return tmp;
}

/**
 * Print an ASON value as a string.
 **/
static char *
ason_do_asprint(ason_t *value, int use_unicode)
{
	switch (value->type) {
	case ASON_NUMERIC:
		return xasprintf("%lld", value->n);
	case ASON_EMPTY:
		if (use_unicode)
			return xasprintf("∘");
		else
			return xasprintf("nil");
	case ASON_NULL:
		if (use_unicode)
			return xasprintf("∅");
		else
			return xasprintf("Nil");
	case ASON_UNIVERSE:
		return xasprintf("U");
	case ASON_WILD:
		return xasprintf("*");
	case ASON_INTERSECT:
	case ASON_QUERY:
	case ASON_COQUERY:
	case ASON_APPEND:
		return ason_asprint_operator(value, use_unicode);
	case ASON_UNION:
		return ason_asprint_union(value, use_unicode);
	case ASON_OBJECT:
	case ASON_UOBJECT:
		return ason_asprint_object(value, use_unicode);
	case ASON_LIST:
		return ason_asprint_list(value, use_unicode);
	default:
		errx(1, "Unreachable statement at %s:%d", __FILE__, __LINE__);
	};
}

/**
 * Print an ASON value as an ASCII string.
 **/
char *
ason_asprint(ason_t *value)
{
	return ason_do_asprint(value, 0);
}

/**
 * Print an ASON value as a unicode string.
 **/
char *
ason_asprint_unicode(ason_t *value)
{
	return ason_do_asprint(value, 1);
}
