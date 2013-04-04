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

#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <err.h>

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

static inline void *xrealloc(void *pt, size_t sz)
{
	void *ret = realloc(pt, sz);

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

static inline void *xstrndup(const char *str, size_t n)
{
	void *ret = strndup(str, n);

	if (! ret)
		errx(1, "Malloc failed");

	return ret;
}

static inline void *xmemdup(const void *str, size_t n)
{
	void *ret = xmalloc(n);

	memcpy(ret, str, n);
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

#endif /* UTIL_H */
