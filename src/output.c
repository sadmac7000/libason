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
	case TYPE_INTERSECT:
		return 1;
	case TYPE_APPEND:
		return 2;
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
	case TYPE_INTERSECT:
		if (use_unicode)
			return "∩";
		else
			return "&";
	case TYPE_APPEND:
		return ":";
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

	if (value->type == TYPE_UOBJECT)
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
	case TYPE_NUMERIC:
		return ason_asprint_number(value->n);
	case TYPE_EMPTY:
		if (use_unicode)
			return xasprintf("∅");
		else
			return xasprintf("_");
	case TYPE_NULL:
		return xasprintf("null");
	case TYPE_TRUE:
		return xasprintf("true");
	case TYPE_FALSE:
		return xasprintf("false");
	case TYPE_STRING:
		tmp = string_escape(value->string);
		ret = xasprintf("\"%s\"", tmp);
		free(tmp);
		return ret;
	case TYPE_UNIVERSE:
		return xasprintf("U");
	case TYPE_WILD:
		return xasprintf("*");
	case TYPE_INTERSECT:
	case TYPE_APPEND:
		return ason_asprint_operator(value, use_unicode);
	case TYPE_UNION:
		return ason_asprint_union(value, use_unicode);
	case TYPE_OBJECT:
	case TYPE_UOBJECT:
		return ason_asprint_object(value, use_unicode);
	case TYPE_LIST:
		return ason_asprint_list(value, use_unicode);
	case TYPE_COMP:
		tmp = ason_do_asprint(value->items[0], use_unicode);
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
	char *str = ason_do_asprint(value, 0);
	char *ret = string_from_utf8(str);

	free(str);
	return ret;
}

/**
 * Print an ASON value as a unicode string.
 **/
API_EXPORT char *
ason_asprint_unicode(ason_t *value)
{
	return ason_do_asprint(value, 1);
}
