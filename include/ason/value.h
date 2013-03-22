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

#ifndef ASONALG_VALUE_H
#define ASONALG_VALUE_H

/**
 * An algebraic ASON type.
 **/
typedef enum {
	ASON_NUMERIC,
	ASON_NULL,
	ASON_STRONG_NULL,
	ASON_UNIVERSE,
	ASON_WILD,
	ASON_UNION,
	ASON_OVERLAP,
	ASON_QUERY,
	ASON_INTERSECT,
	ASON_OBJECT,
	ASON_UOBJECT,
	ASON_LIST,
	ASON_APPEND,
} ason_type_t;

/**
 * An ASON value
 **/
typedef struct ason ason_t;

extern ason_t * const VALUE_NULL;
extern ason_t * const VALUE_STRONG_NULL;
extern ason_t * const VALUE_UNIVERSE;
extern ason_t * const VALUE_WILD;
extern ason_t * const VALUE_OBJ_ANY;

#ifdef __cplusplus
extern "C" {
#endif

ason_t *ason_create_number(int number);
ason_t *ason_create_list(ason_t *content);
ason_t *ason_create_object(const char *key, ason_t *value); 
ason_t *ason_union(ason_t *a, ason_t *b);
ason_t *ason_overlap(ason_t *a, ason_t *b);
ason_t *ason_query(ason_t *a, ason_t *b);
ason_t *ason_intersect(ason_t *a, ason_t *b);
ason_t *ason_append(ason_t *a, ason_t *b);

int ason_check_equality(ason_t *a, ason_t *b);
int ason_check_represented_in(ason_t *a, ason_t *b);
int ason_check_corepresented(ason_t *a, ason_t *b);

ason_type_t ason_type(ason_t *value);

#ifdef __cplusplus
}
#endif

#endif /* ASONALG_VALUE_H */
