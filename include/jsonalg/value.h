/**
 * This file is part of libjsonalg.
 *
 * libjsonalg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libjsonalg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libjsonalg. If not, see <http://www.gnu.org/licenses/>.
 **/

#ifndef JSONALG_VALUE_H
#define JSONALG_VALUE_H

/**
 * An algebraic JSON type.
 **/
typedef enum {
	JSON_NUMERIC,
	JSON_NULL,
	JSON_STRONG_NULL,
	JSON_UNIVERSE,
	JSON_WILD,
	JSON_DISJOIN,
	JSON_INTERSECT,
	JSON_QUERY,
	JSON_COQUERY,
	JSON_OBJECT,
	JSON_UOBJECT,
	JSON_LIST,
	JSON_APPEND,
} json_type_t;

/**
 * A JSON value
 **/
typedef struct json_t {
#ifdef IN_LIBJSONALG
	struct value_data *v;
#else
	void *opaque__;
#endif
} json_t;

extern const json_t VALUE_NULL;
extern const json_t VALUE_STRONG_NULL;
extern const json_t VALUE_UNIVERSE;
extern const json_t VALUE_WILD;
extern const json_t VALUE_OBJ_ANY;

#ifdef __cplusplus
extern "C" {
#endif

json_t json_create_number(int number);
json_t json_create_list(json_t content);
json_t json_create_object(const char *key, json_t value); 
json_t json_disjoin(json_t a, json_t b);
json_t json_intersect(json_t a, json_t b);
json_t json_query(json_t a, json_t b);
json_t json_coquery(json_t a, json_t b);
json_t json_append(json_t a, json_t b);

int json_check_equality(json_t a, json_t b);
int json_check_represented_in(json_t a, json_t b);
int json_check_corepresented(json_t a, json_t b);

json_type_t json_type(json_t value);

#ifdef __cplusplus
}
#endif

#endif /* JSONALG_VALUE_H */
