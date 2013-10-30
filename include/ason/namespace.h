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

#ifndef ASON_NAMESPACE_H
#define ASON_NAMESPACE_H

#include <ason/value.h>

/**
 * A namespace of ASON variables.
 **/
typedef struct ason_ns ason_ns_t;

/**
 * Operations an ASON namespace supports.
 *
 * init: Initialize the namespace.
 * teardown: Tear down the namespace.
 * load: Get the value of a variable.
 * store: Store a value into a variable.
 * create: Create a new variable.
 * set_meta: Set variable metadata. Function is provider-specific.
 * get_meta: Get variable metadata.
 **/
typedef struct ason_ns_ops {
	void *(*init)(const char *setup);
	void (*teardown)(void *data);
	ason_t *(*load)(void *data, const char *name);
	int (*store)(void *data, const char *name, ason_t *value);
	int (*mkvar)(void *data, const char *name);
	int (*set_meta)(void *data, const char *name, const char *meta);
	const char *(*get_meta)(void *data, const char *name);
} ason_ns_ops_t;

extern const ason_ns_ops_t *ASON_NS_RAM;

#ifdef __cplusplus
extern "C" {
#endif

ason_ns_t *ason_ns_create(const ason_ns_ops_t *ops, const char *setup);
void ason_ns_destroy(ason_ns_t *ns);
ason_t *ason_ns_load(ason_ns_t *ns, const char *name);
int ason_ns_store(ason_ns_t *ns, const char *name, ason_t *value);
int ason_ns_mkvar(ason_ns_t *ns, const char *name);
int ason_ns_set_meta(ason_ns_t *ns, const char *name, const char *meta);
const char *ason_ns_get_meta(ason_ns_t *ns, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* ASON_NAMESPACE_H */
