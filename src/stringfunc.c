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

#include <iconv.h>
#include <err.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "stringfunc.h"
#include "util.h"

/**
 * Default locale string buffer length.
 **/
static size_t
default_str_buflen(const char *str)
{
	return strlen(str) + 1;
}

const char *input_locale;
const char *output_locale;
size_t (*locale_str_buflen)(const char *) = default_str_buflen;

/**
 * Open an iconv_t descriptor. Always succeed.
 **/
static iconv_t
xiconv_open(const char *to, const char *from)
{
	iconv_t ret = iconv_open(to, from);

	if (ret == (iconv_t)-1)
		err(1, "Could not get iconv state");

	return ret;
}

/**
 * Initialize the values of output_locale and input_locale.
 **/
static void
setup_locales(void)
{
	if (! output_locale)
		output_locale = "UTF-8";

	if (! input_locale)
		input_locale = "UTF-8";
}

/**
 * Get an iconv_t for converting input strings.
 **/
static iconv_t
get_input_iconv(void)
{
	setup_locales();
	return xiconv_open("UTF-8", input_locale);
}

/**
 * Get an iconv_t for converting output strings.
 **/
static iconv_t
get_output_iconv(void)
{
	setup_locales();
	return xiconv_open(output_locale, "UTF-8");
}

/**
 * Run iconv and convert a string of known length.
 **/
static char *
string_do_convert_length(const char *in, iconv_t ic, size_t in_sz)
{
	char *my_in = xstrdup(in);
	char *my_in_mem = my_in;
	char *out = xmalloc(6 * in_sz); /* Max UTF-8 expansion */
	char *ret = out;
	size_t in_bytes = in_sz;
	size_t out_bytes_start = 6 * in_sz + 1;
	size_t out_bytes = out_bytes_start;
	size_t got;

	got = iconv(ic, &my_in, &in_bytes, &out, &out_bytes);

	if (in_bytes)
		err(1, "String conversion left %zd bytes, returned %zd",
		    in_bytes, got);

	free(my_in_mem);

	return xrealloc(ret, out_bytes_start - out_bytes);
}

/**
 * Run iconv and convert a string.
 **/
static char *
string_do_convert(const char *in, iconv_t ic)
{
	size_t in_sz = locale_str_buflen(in);

	return string_do_convert_length(in, ic, in_sz);
}

/**
 * Convert a string from our input locale to UTF-8
 **/
char *
string_to_utf8(const char *in)
{
	iconv_t ic = get_input_iconv();
	char *ret = string_do_convert(in, ic);

	iconv_close(ic);
	return ret;
}

/**
 * Convert a string from UTF-8 to our output locale.
 **/
char *
string_from_utf8(const char *in)
{
	iconv_t ic = get_output_iconv();
	char *ret = string_do_convert(in, ic);

	iconv_close(ic);
	return ret;
}

/**
 * Get an escaped version of a string.
 **/
char *
string_escape(const char *in)
{
	/* There's a char32_t in newer C standards. Use it? */
	uint32_t *expanded_buf;
	size_t chars = strlen(in);
	iconv_t ic = xiconv_open("UTF-32", "UTF-8");
	char *ret;
	char *pos;
	size_t i;

	expanded_buf = (uint32_t *)string_do_convert(in, ic);

	iconv_close(ic);

	/* Again, max expansion, for a different reason. We can represent a
	 * single byte character as '\xb33f'. */
	ret = xmalloc(6 * chars + 1);
	pos = ret;

	for (i = 0; i < chars; i++) {
		if ((expanded_buf[i] & 0xffffff80) ||
		    iscntrl(expanded_buf[i])) {
			pos += sprintf(pos, "\\u%04x", expanded_buf[i]);
			continue;
		}

		switch (expanded_buf[i]) {
		case '\"':
			pos += sprintf(pos, "\\\"");
			break;
		case '\\':
			pos += sprintf(pos, "\\\\");
			break;
		case '/':
			pos += sprintf(pos, "\\/");
			break;
		case '\b':
			pos += sprintf(pos, "\\b");
			break;
		case '\f':
			pos += sprintf(pos, "\\f");
			break;
		case '\n':
			pos += sprintf(pos, "\\n");
			break;
		case '\r':
			pos += sprintf(pos, "\\r");
			break;
		case '\t':
			pos += sprintf(pos, "\\t");
			break;
		default:
			pos += sprintf(pos, "%c", (char)expanded_buf[i]);
		};
	}

	*pos = '\0';
	free(expanded_buf);
	return xrealloc(ret, strlen(ret));
}

/**
 * Unescape an escaped string.
 **/
char *
string_unescape(const char *in)
{
	size_t len = strlen(in) + 1;
	iconv_t ic = xiconv_open("UTF-32", "UTF-8");
	uint32_t *in_exp = (uint32_t *)string_do_convert(in, ic);
	uint32_t *out_exp = xcalloc(len, sizeof(uint32_t));
	uint32_t *in_pos = in_exp;
	uint32_t *out_pos = out_exp;
	char tmp_str[5];
	char *ret;
	size_t i;

	iconv_close(ic);

	for(; *in_pos; out_pos++, in_pos++) {
		if (*in_pos != '\\') {
			*out_pos = *in_pos;
			continue;
		}

		in_pos++;

		switch (*in_pos) {
		case '\"':
		case '\\':
		case '/':
			*out_pos = *in_pos;
			break;
		case 'b':
			*out_pos = '\b';
			break;
		case 'f':
			*out_pos = '\f';
			break;
		case 'n':
			*out_pos = '\n';
			break;
		case 'r':
			*out_pos = '\r';
			break;
		case 't':
			*out_pos = '\t';
			break;
		case 'u':
			for (i = 0; i < 4; i++)
				tmp_str[i] = (char)*(++in_pos);

			tmp_str[4] = '\0';

			if (sscanf(tmp_str, "%04x", out_pos) > 0)
				break;
		default:
			errx(1, "Unexpected escape sequence");
		}
	}

	*out_pos = 0;

	ic = xiconv_open("UTF-8", "UTF-32");

	ret = string_do_convert_length((char *)out_exp, ic, len);

	iconv_close(ic);

	free(out_exp);
	free(in_exp);

	return ret;
}
