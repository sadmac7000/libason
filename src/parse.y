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

%type file      {ason_t *}
%type value     {ason_t *}
%type list      {ason_t *}
%type kv_list   {ason_t *}
%type kv_pair   {ason_t *}
%type append    {ason_t *}
%type query     {ason_t *}
%type intersect {ason_t *}
%type coquery   {ason_t *}
%type union     {ason_t *}

%destructor value { ason_destroy($$); }
%destructor list { ason_destroy($$); }
%destructor kv_list { ason_destroy($$); }
%destructor kv_pair { ason_destroy($$); }

%name asonLemon
%token_prefix ASON_LEX_

result ::= union(A). { *ret = A; }

union(A) ::= coquery(B).				{ A = B; }
union(A) ::= union(B) UNION coquery(C).	{
	A = ason_union(B, C);
	ason_destroy(B);
	ason_destroy(C);
}

coquery(A) ::= intersect(B).				{ A = B; }
coquery(A) ::= coquery(B) COQUERY intersect(C).	{
	A = ason_coquery(B, C);
	ason_destroy(B);
	ason_destroy(C);
}

intersect(A) ::= query(B).				{ A = B; }
intersect(A) ::= intersect(B) INTERSECT query(C).	{
	A = ason_intersect(B, C);
	ason_destroy(B);
	ason_destroy(C);
}

query(A) ::= append(B).				{ A = B; }
query(A) ::= query(B) QUERY append(C).	{
	A = ason_query(B, C);
	ason_destroy(B);
	ason_destroy(C);
}

append(A) ::= value(B).				{ A = B; }
append(A) ::= value(B) APPEND append(C).	{
	A = ason_append(B, C);
	ason_destroy(B);
	ason_destroy(C);
}

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

list(A) ::= union(B).				{ A = ason_create_list(B); }
list(A) ::= union(B) COMMA list(C).		{
	A = ason_append(B, C);
	ason_destroy(B);
	ason_destroy(C);
}

kv_pair(A) ::= STRING(B) COLON union(C).	{
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
