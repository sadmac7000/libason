%include {
#include <ason/value.h>
#include <assert.h>
#include <stdlib.h>

#include "value.h"

typedef union {
	int64_t n;
	char *c;
} token_t;

/* Lemon has a problem with these */
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"

}

%token_type {token_t}

%extra_argument {ason_t **ret}

%left UNION.
%left INTERSECT.
%left APPEND.
%left COMMA.

%type file      {ason_t *}
%type value     {ason_t *}
%type list      {ason_t *}
%type kv_list   {ason_t *}
%type kv_pair   {ason_t *}
%type append    {ason_t *}
%type intersect {ason_t *}
%type union     {ason_t *}
%type comp      {ason_t *}

%destructor value { ason_destroy($$); }
%destructor list { ason_destroy($$); }
%destructor kv_list { ason_destroy($$); }
%destructor kv_pair { ason_destroy($$); }

%name asonLemon
%token_prefix ASON_LEX_

result ::= union(A). { *ret = A; }

union(A) ::= intersect(B).				{ A = B; }
union(A) ::= union(B) UNION intersect(C).	{
	A = ason_union_d(B, C);
}

intersect(A) ::= comp(B).				{ A = B; }
intersect(A) ::= intersect(B) INTERSECT comp(C).	{
	A = ason_intersect_d(B, C);
}

comp(A) ::= append(B). { A = B; }
comp(A) ::= NOT comp(B). { A = ason_complement_d(B); }

append(A) ::= value(B).				{ A = B; }
append(A) ::= value(B) APPEND append(C).	{
	A = ason_append_d(B, C);
}

value(A) ::= EMPTY.		{ A = VALUE_EMPTY; }
value(A) ::= NULL.		{ A = VALUE_NULL; }
value(A) ::= UNIVERSE.		{ A = VALUE_UNIVERSE; }
value(A) ::= WILD.		{ A = VALUE_WILD; }

value(A) ::= NUMBER(B).		{ A = ason_create_fixnum(B.n); }

value(A) ::= START_LIST list(B) END_LIST.		{ A = B; }
value(A) ::= TRUE.					{ A = VALUE_TRUE; }
value(A) ::= FALSE.					{ A = VALUE_FALSE; }
value(A) ::= START_OBJ kv_list(B) END_OBJ.		{ A = B; }
value(A) ::= START_OBJ kv_list(B) COMMA WILD END_OBJ.	{
	A = ason_append_d(B, VALUE_OBJ_ANY);
}
value(A) ::= STRING(B). {
	A = ason_create_string(B.c);
	free(B.c);
}

value(A) ::= O_PAREN union(B) C_PAREN. { A = B; }

list(A) ::= union(B).				{ A = ason_create_list_d(B); }
list(A) ::= union(B) COMMA list(C).		{
	A = ason_append_d(ason_create_list_d(B), C);
}

kv_pair(A) ::= STRING(B) COLON union(C).	{
	A = ason_create_object_d(B.c, C);
	free(B.c);
}

kv_list(A) ::= kv_pair(B).			{ A = B; }
kv_list(A) ::= kv_list(B) COMMA kv_pair(C).	{
	A = ason_append_d(B, C);
}
