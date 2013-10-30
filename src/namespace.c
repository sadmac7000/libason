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

#include <ason/namespace.h>

#include "util.h"

/**
 * An ASON namespace.
 **/
struct ason_ns {
	const ason_ns_ops_t *ops;
	void *data;
};

/**
 * Create an ASON namespace.
 **/
API_EXPORT ason_ns_t *
ason_ns_create(const ason_ns_ops_t *ops, const char *setup)
{
	ason_ns_t *ns = xmalloc(sizeof(ason_ns_t));

	ns->ops = ops;
	ns->data = ns->ops->init(setup);

	return ns;
}

/**
 * Load a value from an ASON namespace.
 **/
API_EXPORT ason_t *
ason_ns_load(ason_ns_t *ns, const char *name)
{
	return ns->ops->load(ns->data, name);
}

/**
 * Store a value into an ASON namespace.
 **/
API_EXPORT int
ason_ns_store(ason_ns_t *ns, const char *name, ason_t *value)
{
	return ns->ops->store(ns->data, name, value);
}

/**
 * Create a value in an ASON namespace.
 **/
API_EXPORT int
ason_ns_mkvar(ason_ns_t *ns, const char *name)
{
	return ns->ops->mkvar(ns->data, name);
}

/**
 * Set metadata for a variable in an ASON namespace.
 **/
API_EXPORT int
ason_ns_set_meta(ason_ns_t *ns, const char *name, const char *meta)
{
	return ns->ops->set_meta(ns->data, name, meta);
}

/**
 * Get metadata fro a variable in an ASON namespace.
 **/
const char *
ason_ns_get_meta(ason_ns_t *ns, const char *name)
{
	return ns->ops->get_meta(ns->data, name);
}
