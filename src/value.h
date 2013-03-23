/**
 * This file is part of libasonalg.
 *
 * libasonalg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libasonalg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libasonalg. If not, see <http://www.gnu.org/licenses/>.
 **/

#ifndef VALUE_H
#define VALUE_H

#include <stdio.h>
#include <stdarg.h>

#include <ason/value.h>

/**
 * A Key-value pair.
 **/
struct kv_pair {
	char *key;
	ason_t *value;
};

/**
 * Data making up a value.
 **/
struct ason {
	ason_type_t type;
	union {
		int64_t n;
		uint64_t u;
		ason_t **items;
		struct kv_pair *kvs;
	};

	size_t count;
	size_t refcount;
};

/**
 * Test if an ASON value is an object.
 **/
#define IS_OBJECT(_x) (_x->type == ASON_OBJECT || _x->type == ASON_UOBJECT)
#define IS_NULL(_x) (_x->type == ASON_NULL || _x->type == ASON_STRONG_NULL)

#ifdef __cplusplus
extern "C" {
#endif

static inline void *xmalloc(size_t sz)
{
	void *ret = malloc(sz);

	if (! ret)
		errx(1, "Malloc failed");

	return ret;
}

static inline void *xcalloc(size_t memb, size_t sz)
{
	void *ret = calloc(memb, sz);

	if (! ret)
		errx(1, "Malloc failed");

	return ret;
}

static inline void *xstrdup(const char *str)
{
	void *ret = strdup(str);

	if (! ret)
		errx(1, "Malloc failed");

	return ret;
}

static inline char *xasprintf(const char *fmt, ...)
{
	char *ret;
	int got;
	va_list ap;

	va_start(ap, fmt);
	got = vasprintf(&ret, fmt, ap);
	va_end(ap);

	if (got < 0)
		errx(1, "Malloc failed");

	return ret;
}

#ifdef __cplusplus
}
#endif

#endif /* VALUE_H */
