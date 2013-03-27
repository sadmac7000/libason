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
#line 18 "parse.append.c"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <ason/read.h>

#include "parse.h"
#include "util.h"

/**
 * Read an ASON value from a string.
 **/
ason_t *
ason_read(const char *text)
{
	return ason_readn(text, strlen(text));
}

/**
 * Tokenize a string for ASON parsing.
 **/
static size_t
ason_get_token(const char *text, size_t length, int *type, token_t *data)
{
	const char *text_start = text;
	const char *tok_start;
	char *tmp;

	while (length && isspace(*text)) {
		length--;
		text++;
	}

	if (! length)
		return 0;

#define FIXED_TOKEN(s, u) do { \
	if (strlen(s) < length && !strncmp(s, text, strlen(s))) { \
		text += strlen(s); \
		*type = u; \
		return text - text_start; \
	}  } while (0)

	FIXED_TOKEN("|", ASON_LEX_UNION);
	FIXED_TOKEN("∪", ASON_LEX_UNION);
	FIXED_TOKEN("~~", ASON_LEX_COQUERY);
	FIXED_TOKEN("⋈", ASON_LEX_COQUERY);
	FIXED_TOKEN("&", ASON_LEX_INTERSECT);
	FIXED_TOKEN("∩", ASON_LEX_INTERSECT);
	FIXED_TOKEN("~", ASON_LEX_QUERY);
	FIXED_TOKEN("⊳", ASON_LEX_QUERY);
	FIXED_TOKEN(".", ASON_LEX_APPEND);
	FIXED_TOKEN(",", ASON_LEX_COMMA);
	FIXED_TOKEN("∘", ASON_LEX_NIL);
	FIXED_TOKEN("nil", ASON_LEX_NIL);
	FIXED_TOKEN("∅", ASON_LEX_NIL_STRONG);
	FIXED_TOKEN("Nil", ASON_LEX_NIL_STRONG);
	FIXED_TOKEN("U", ASON_LEX_UNIVERSE);
	FIXED_TOKEN("*", ASON_LEX_WILD);
	FIXED_TOKEN("[", ASON_LEX_START_LIST);
	FIXED_TOKEN("]", ASON_LEX_END_LIST);
	FIXED_TOKEN("{", ASON_LEX_START_OBJ);
	FIXED_TOKEN("}", ASON_LEX_END_OBJ);
	FIXED_TOKEN(":", ASON_LEX_COLON);

#undef FIXED_TOKEN

	tok_start = text;

	while (length && isdigit(*text)) {
		text++;
		length--;
	}

	if (tok_start != text) {
		tmp = xmalloc(text - tok_start + 1);
		tmp[text - tok_start] = '\0';
		memcpy(tmp, tok_start, text - tok_start);
		sscanf(tmp, "%d", &data->i);
		free(tmp);

		*type = ASON_LEX_INTEGER;
		return text - text_start;
	}

	if (*text != '"')
		return 0;

	tok_start = ++text;

	while (length && *text != '"' && *(text - 1) != '\\') {
		length--;
		text++;
	}

	if (*text != '"' || *(text - 1) == '\\')
		return 0;

	data->c = xstrndup(tok_start, text - tok_start);
	text++;
	*type = ASON_LEX_STRING;
	return text - text_start;
}

/**
 * Read an ASON value from a string. Stop after `length` bytes.
 **/
ason_t *
ason_readn(const char *text, size_t length)
{
	token_t data;
	size_t len;
	int type;
	void *parser = asonLemonAlloc(xmalloc);
	ason_t *ret = NULL;

	while ((len = ason_get_token(text, length, &type, &data))) {
		text += len;

		asonLemon(parser, type, data, &ret);
	}

	asonLemon(parser, 0, data, &ret);
	asonLemonFree(parser, free);
	
	return ret;
}
