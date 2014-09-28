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
#include "util.h"
#include "stringfunc.h"

/* Predeclaration */
static char *ason_do_asprint(ason_t *value, int use_unicode);

/**
 * Get a precedence value for an operator.
 **/
static int
ason_get_precedence(ason_type_t operator)
{
	switch (operator) {
	case ASON_TYPE_EQUAL:
		return 1;
	case ASON_TYPE_REPR:
		return 2;
	case ASON_TYPE_UNION:
		return 3;
	case ASON_TYPE_INTERSECT:
		return 4;
	case ASON_TYPE_JOIN:
		return 5;
	default:
		return INT_MAX;
	}
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
	char *key;
	size_t i;

	/* FIXME: Escaping for keys. */
	for (i = 0; i < value->count; i++) {
		next = ason_do_asprint(value->kvs[i].value, use_unicode);

		tmp = next;
		key = string_escape(value->kvs[i].key);
		next = xasprintf("\"%s\": %s", key, next);
		free(tmp);
		free(key);

		if (out) {
			tmp = out;
			out = xasprintf("%s, %s", out, next);
			free(tmp);
			free(next);
		} else {
			out = next;
		}
	}

	if (out && value->type == ASON_TYPE_UOBJECT)
		tmp = xasprintf("{ %s, *}", out);
	else if (out)
		tmp = xasprintf("{ %s }", out);
	else if (value->type == ASON_TYPE_UOBJECT)
		tmp = xasprintf("{*}");
	else
		tmp = xasprintf("{}");
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
 * Print a fixed-point number as a string.
 **/
static char *
ason_asprint_number(int64_t num)
{
	char *ret = xasprintf("%lld", FP_WHOLE(num));
	char *tmp;

	if (num < 0)
		num = -num;

	num -= TO_FP(FP_WHOLE(num));

	if (! num)
		return ret;

	tmp = ret;
	ret = xasprintf("%s.", tmp);
	free(tmp);

	while (num) {
		num *= 10;
		tmp = ret;
		ret = xasprintf("%s%lld", tmp, FP_WHOLE(num));
		free(tmp);
		num -= TO_FP(FP_WHOLE(num));
	}

	return ret;
}

/**
 * Print an ASON value as a string.
 **/
static char *
ason_do_asprint(ason_t *value, int use_unicode)
{
	char *tmp;
	char *ret;

	switch (value->type) {
	case ASON_TYPE_NUMERIC:
		return ason_asprint_number(value->n);
	case ASON_TYPE_EMPTY:
		if (use_unicode)
			return xasprintf("∅");
		else
			return xasprintf("_");
	case ASON_TYPE_NULL:
		return xasprintf("null");
	case ASON_TYPE_TRUE:
		return xasprintf("true");
	case ASON_TYPE_FALSE:
		return xasprintf("false");
	case ASON_TYPE_STRING:
		tmp = string_escape(value->string);
		ret = xasprintf("\"%s\"", tmp);
		free(tmp);
		return ret;
	case ASON_TYPE_INTERSECT:
	case ASON_TYPE_JOIN:
	case ASON_TYPE_REPR:
	case ASON_TYPE_EQUAL:
		errx(1, "Value is not reduced in asprint");
	case ASON_TYPE_UNION:
		return ason_asprint_union(value, use_unicode);
	case ASON_TYPE_OBJECT:
	case ASON_TYPE_UOBJECT:
		return ason_asprint_object(value, use_unicode);
	case ASON_TYPE_LIST:
		return ason_asprint_list(value, use_unicode);
	case ASON_TYPE_COMP:
		if (value->items[0]->type == ASON_TYPE_NULL)
			return xasprintf("*");
		if (value->items[0]->type == ASON_TYPE_EMPTY)
			return xasprintf("U");
		tmp = ason_do_asprint(value->items[0], use_unicode);
		if (ason_get_precedence(value->items[0]->type) < INT_MAX)
			ret = xasprintf("!( %s )", tmp);
		else
			ret = xasprintf("!%s", tmp);
		free(tmp);
		return ret;
	default:
		errx(1, "Unreachable statement at %s:%d", __FILE__, __LINE__);
	};
}

/**
 * Print an ASON value as an ASCII string.
 **/
API_EXPORT char *
ason_asprint(ason_t *value)
{
	char *str;
	char *ret;

	ason_reduce(value);

	str = ason_do_asprint(value, 0);
	ret = string_from_utf8(str);
	free(str);

	return ret;
}

/**
 * Print an ASON value as a unicode string.
 **/
API_EXPORT char *
ason_asprint_unicode(ason_t *value)
{
	ason_reduce(value);
	return ason_do_asprint(value, 1);
}
