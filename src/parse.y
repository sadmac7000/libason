%include {
#include <ason/value.h>
#include <assert.h>
typedef union {
	int i;
	char *c;
} token_t;
}

%token_type {token_t}

%extra_argument {ason_t **ret}

%left UNION.
%left COQUERY.
%left INTERSECT.
%left QUERY.
%left APPEND.
%left COMMA.

%type file    {ason_t *}
%type value   {ason_t *}
%type list    {ason_t *}
%type kv_list {ason_t *}
%type kv_pair {ason_t *}

%destructor value { ason_destroy($$); }
%destructor list { ason_destroy($$); }
%destructor kv_list { ason_destroy($$); }
%destructor kv_pair { ason_destroy($$); }

%name asonLemon
%token_prefix ASON_LEX_

result ::= value(A). { *ret = A; }

value(A) ::= NIL.		{ A = VALUE_NULL; }
value(A) ::= NIL_STRONG.	{ A = VALUE_STRONG_NULL; }
value(A) ::= UNIVERSE.		{ A = VALUE_UNIVERSE; }
value(A) ::= WILD.		{ A = VALUE_WILD; }

value(A) ::= INTEGER(B).	{ A = ason_create_number(B.i); }

value(A) ::= START_LIST list(B) END_LIST.		{ A = B; }
value(A) ::= START_OBJ kv_list(B) END_OBJ.		{ A = B; }
value(A) ::= START_OBJ kv_list(B) COMMA WILD END_OBJ.	{
	A = ason_append(B, VALUE_OBJ_ANY);
	ason_destroy(B);
}

list(A) ::= value(B).				{ A = B; }
list(A) ::= value(B) COMMA list(C).		{
	A = ason_append(B, C);
	ason_destroy(B);
	ason_destroy(C);
}

kv_pair(A) ::= STRING(B) COLON value(C).	{
	A = ason_create_object(B.c, C);
	free(B.c);
	ason_destroy(C);
}

kv_list(A) ::= kv_pair(B).			{ A = B; }
kv_list(A) ::= kv_list(B) COMMA kv_pair(C).	{
	A = ason_append(B, C);
	ason_destroy(B);
	ason_destroy(C);
}
