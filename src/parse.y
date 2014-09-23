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

%include {
#include <ason/value.h>
#include <ason/read.h>

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

#include "value.h"

typedef union {
	int64_t n;
	char *c;
	ason_t *value;
} token_t;

/**
 * Output data.
 **/
struct parse_data {
	ason_t *ret;
	ason_ns_t *ns;
	int failed;
};

/* Lemon has a problem with these */
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"

}

%token_type {token_t}

%extra_argument {struct parse_data *data}

%syntax_error {data->failed = 1;}

%right ASSIGN.
%right EQUAL.
%right SUBSET.

%left UNION.
%left INTERSECT.
%left COLON.
%left COMMA.

%type value     {ason_t *}
%type list      {ason_t *}
%type kv_list   {ason_t *}
%type kv_pair   {ason_t *}
%type join    {ason_t *}
%type intersect {ason_t *}
%type union     {ason_t *}
%type comp      {ason_t *}
%type equality  {ason_t *}
%type repr      {ason_t *}

%destructor value     { ason_destroy($$); }
%destructor list      { ason_destroy($$); }
%destructor kv_list   { ason_destroy($$); }
%destructor kv_pair   { ason_destroy($$); }
%destructor join      { ason_destroy($$); }
%destructor intersect { ason_destroy($$); }
%destructor union     { ason_destroy($$); }
%destructor comp      { ason_destroy($$); }
%destructor equality  { ason_destroy($$); }
%destructor repr      { ason_destroy($$); }

%name asonLemon
%token_prefix ASON_LEX_

assignment ::= result.
assignment ::= SYMBOL(A) ASSIGN result.			{
	int i = ason_ns_mkvar(data->ns, A.c);

	if (!i || i == -EEXIST)
		ason_ns_store(data->ns, A.c, data->ret);
	else
		data->failed = 1;

	free(A.c);
}

result ::= equality(A). { data->ret = A; }

equality(A) ::= repr(B).				{ A = B; }
equality(A) ::= equality(B) EQUAL repr(C).		{
	A = ason_equality_d(B, C);
}

repr(A) ::= union(B).					{ A = B; }
repr(A) ::= repr(B) REPR union(C).		{
	A = ason_representation_in_d(B, C);
}


union(A) ::= intersect(B).				{ A = B; }
union(A) ::= union(B) UNION intersect(C).	{
	A = ason_union_d(B, C);
}

intersect(A) ::= comp(B).				{ A = B; }
intersect(A) ::= intersect(B) INTERSECT comp(C).	{
	A = ason_intersect_d(B, C);
}

comp(A) ::= join(B). { A = B; }
comp(A) ::= NOT comp(B). { A = ason_complement_d(B); }

join(A) ::= value(B).				{ A = B; }
join(A) ::= value(B) COLON join(C).		{
	A = ason_join_d(B, C);
}

value(A) ::= PREBAKED(B).	{ A = B.value; }
value(A) ::= EMPTY.		{ A = ASON_EMPTY; }
value(A) ::= NULL.		{ A = ASON_NULL; }
value(A) ::= UNIVERSE.		{ A = ASON_UNIVERSE; }
value(A) ::= WILD.		{ A = ASON_WILD; }

value(A) ::= NUMBER(B).		{ A = ason_create_fixnum(B.n); }

value(A) ::= START_LIST END_LIST.			{
	A = ason_create_list(NULL);
}

value(A) ::= START_LIST list(B) END_LIST.		{ A = B; }
value(A) ::= TRUE.					{ A = ASON_TRUE; }
value(A) ::= FALSE.					{ A = ASON_FALSE; }
value(A) ::= START_OBJ END_OBJ.				{
	A = ason_create_object(NULL,NULL);
}

value(A) ::= START_OBJ kv_list(B) END_OBJ.		{ A = B; }
value(A) ::= START_OBJ kv_list(B) COMMA WILD END_OBJ.	{
	A = ason_join_d(B, ASON_OBJ_ANY);
}
value(A) ::= START_OBJ WILD END_OBJ.			{ A = ASON_OBJ_ANY; }
value(A) ::= STRING(B). {
	A = ason_create_string(B.c);
	free(B.c);
}

value(A) ::= O_PAREN union(B) C_PAREN. { A = B; }
value(A) ::= SYMBOL(B). {
	if (data->ns)
		A = ason_ns_load(data->ns, B.c) ?: ASON_EMPTY;
	else
		A = ASON_EMPTY;
}

list(A) ::= union(B).				{ A = ason_create_list_d(B); }
list(A) ::= union(B) COMMA list(C).		{
	A = ason_append_lists_d(ason_create_list_d(B), C);
}

kv_pair(A) ::= STRING(B) COLON union(C).	{
	A = ason_create_object_d(B.c, C);
	free(B.c);
}

kv_list(A) ::= kv_pair(B).			{ A = B; }
kv_list(A) ::= kv_list(B) COMMA kv_pair(C).	{
	A = ason_join_d(B, C);
}

%code {

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "parse.h"
#include "util.h"
#include "stringfunc.h"

/**
 * Get a number token.
 **/
static const char *
ason_get_token_number(const char *text, size_t length, int *type,
		      token_t *data)
{
	const char *text_start;
	int negator = 1;
	size_t decimal_places = 0;
	size_t decimal_inc = 0;
	size_t digits = 0;
	int64_t accum = 0;

	if (! length)
		return text;

	text_start = text;

	if (*text == '-') {
		negator = -1;
		text++;
		length--;
	}

	if (! length)
		return text_start;

	if (length < 2 && *text == 0)
		return text_start;

	if (text[0] == '0' && text[1] != '.')
		return text_start;

	for (; length; length--, text++) {
		if (*text == '.') {
			if (decimal_inc)
				return text_start;
			decimal_inc = 1;
			continue;
		} else if (! isdigit(*text)) {
			break;
		}

		digits++;
		decimal_places += decimal_inc;
		accum *= 10;
		accum += (*text) - '0';
	}

	if (decimal_inc && !decimal_places)
		return text_start;

	accum *= negator;

	/* Technically << can be undefined for negative numbers in C */
	accum = TO_FP(accum);

	while (decimal_places--)
		accum /= 10;

	if (text == text_start)
		return text_start;

	*type = ASON_LEX_NUMBER;
	data->n = accum;
	return text;
}

/**
 * Get a token from a positional argument.
 **/
static size_t
ason_get_token_arg(char type, token_t *data, va_list ap)
{
	size_t ret = 1;

	switch (type) {
	case 'i':
		data->value = ason_create_fixnum(0);
		data->value->n = TO_FP(va_arg(ap, int));
		break;
	case 'u':
		data->value = ason_create_fixnum(0);
		data->value->n = TO_FP(va_arg(ap, unsigned int));
		break;
	case 'I':
		data->value = ason_create_fixnum(0);
		data->value->n = TO_FP(va_arg(ap, int64_t));
		break;
	case 'U':
		data->value = ason_create_fixnum(0);
		data->value->n = TO_FP(va_arg(ap, uint64_t));
		break;

	/* Float becomes double in va_arg, so we can handle these together */
	case 'f':
	case 'F':
		data->value = ason_create_fixnum(0);
		/* TO_FP was designed for integers, but this seems to work */
		data->value->n = TO_FP(va_arg(ap, double));
		break;
	case 's':
		data->value = ason_create_string(va_arg(ap, char *));
		break;
	default:
		ret = 0;
		data->value = ason_copy(va_arg(ap, ason_t *));
	};

	return ret;
}

/**
 * Tokenize a string for ASON parsing.
 **/
static size_t
ason_get_token(const char *text, size_t length, int *type, token_t *data,
	       ason_ns_t *ns, va_list ap)
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
	if (strlen(s) <= length && !strncmp(s, text, strlen(s))) { \
		text += strlen(s); \
		*type = u; \
		return text - text_start; \
	}  } while (0)

	FIXED_TOKEN("|", ASON_LEX_UNION);
	FIXED_TOKEN("∪", ASON_LEX_UNION);
	FIXED_TOKEN("&", ASON_LEX_INTERSECT);
	FIXED_TOKEN("∩", ASON_LEX_INTERSECT);
	FIXED_TOKEN(",", ASON_LEX_COMMA);
	FIXED_TOKEN("null", ASON_LEX_NULL);
	FIXED_TOKEN("∅", ASON_LEX_EMPTY);
	FIXED_TOKEN("_", ASON_LEX_EMPTY);
	FIXED_TOKEN("U", ASON_LEX_UNIVERSE);
	FIXED_TOKEN("*", ASON_LEX_WILD);
	FIXED_TOKEN("[", ASON_LEX_START_LIST);
	FIXED_TOKEN("]", ASON_LEX_END_LIST);
	FIXED_TOKEN("{", ASON_LEX_START_OBJ);
	FIXED_TOKEN("}", ASON_LEX_END_OBJ);
	FIXED_TOKEN(":=", ASON_LEX_ASSIGN);
	FIXED_TOKEN(":", ASON_LEX_COLON);
	FIXED_TOKEN("(", ASON_LEX_O_PAREN);
	FIXED_TOKEN(")", ASON_LEX_C_PAREN);
	FIXED_TOKEN("!", ASON_LEX_NOT);
	FIXED_TOKEN("in", ASON_LEX_REPR);
	FIXED_TOKEN("⊆", ASON_LEX_REPR);
	FIXED_TOKEN("=", ASON_LEX_EQUAL);
	FIXED_TOKEN("true", ASON_LEX_TRUE);
	FIXED_TOKEN("false", ASON_LEX_FALSE);

#undef FIXED_TOKEN

	if (*text == '?') {
		text++;
		length--;
		if (length)
			text += ason_get_token_arg(*text, data, ap);
		else
			ason_get_token_arg('\0', data, ap);

		*type = ASON_LEX_PREBAKED;
		return text - text_start;
	}

	tok_start = text;
	text = ason_get_token_number(text, length, type, data);

	if (text > tok_start)
		return text - text_start;

	if (*text != '"') {
		if (isdigit(*text))
			return 0;

		if (! ns)
			return 0;

		tok_start = text;

		while (length && (isalpha(*text) || isdigit(*text) ||
				  *text == '_' || *text == '.')) {
			length--;
			text++;
		}

		if (text == tok_start)
			return 0;

		data->c = xstrndup(tok_start, text - tok_start);
		*type = ASON_LEX_SYMBOL;
		return text - text_start;
	}

	tok_start = ++text;

	while (length && (*text != '"' || *(text - 1) == '\\')) {
		length--;
		text++;
	}

	if (*text != '"' || *(text - 1) == '\\')
		return 0;

	tmp = xstrndup(tok_start, text - tok_start);
	data->c = string_unescape(tmp);
	free(tmp);
	text++;
	*type = ASON_LEX_STRING;
	return text - text_start;
}

/**
 * Read an ASON value from a string. Stop after `length` bytes. Use `ns` to
 * resolve and assign symbols, and `ap` to resolve tokens.
 **/
static ason_t *
ason_vreadn(const char *text, size_t length, ason_ns_t *ns, va_list ap)
{
	token_t data;
	size_t len;
	int type;
	void *parser = asonLemonAlloc(xmalloc);
	struct parse_data pdata = { .ret = NULL, .ns = ns, .failed = 0 };
	char *tmp = xstrndup(text, length);
	char *text_unicode = string_to_utf8(tmp);

	free(tmp);
	text = text_unicode;

	while ((len = ason_get_token(text, length, &type, &data, ns, ap))) {
		text += len;
		length -= len;

		asonLemon(parser, type, data, &pdata);
	}

	while (isspace(*text)) {
		text++;
		length--;
	}

	if (length)
		pdata.failed = 1;

	asonLemon(parser, 0, data, &pdata);
	asonLemonFree(parser, free);

	free(text_unicode);

	if (! pdata.failed)
		return pdata.ret;

	if (pdata.ret)
		ason_destroy(pdata.ret);

	return NULL;
}

/**
 * Read an ASON value from a string. Stop after `length` bytes. Use `ns` to
 * resolve and assign symbols.
 **/
API_EXPORT ason_t *
ason_readn(const char *text, size_t length, ason_ns_t *ns, ...)
{
	va_list ap;
	ason_t *ret;

	va_start(ap, ns);
	ret = ason_vreadn(text, length, ns, ap);
	va_end(ap);
	return ret;
}

/**
 * Read an ASON value from a string.
 **/
API_EXPORT ason_t *
ason_read(const char *text, ason_ns_t *ns, ...)
{
	va_list ap;
	ason_t *ret;

	va_start(ap, ns);
	ret = ason_vreadn(text, strlen(text), ns, ap);
	va_end(ap);
	return ret;
}

}
