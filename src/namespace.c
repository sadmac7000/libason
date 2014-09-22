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

#include <ctype.h>
#include <errno.h>

#include <ason/namespace.h>

#include "util.h"

/**
 * An entry in our registry of types of ASON namespaces.
 */
struct ason_ns_ops_registry {
	char *name;
	const ason_ns_ops_t *ops;
};

/**
 * Our registry of types of ASON namespaces.
 **/
struct ason_ns_ops_registry *ason_ns_ops_registry = NULL;
size_t ason_ns_ops_registry_count = 0;

/**
 * A sub-namespace of an ASON namespace.
 **/
struct ason_subns {
	char *name;
	ason_ns_t *space;
};

/**
 * An ASON namespace.
 **/
struct ason_ns {
	struct ason_ns *parent;
	const ason_ns_ops_t *ops;
	void *data;
	struct ason_subns *subns;
	size_t subns_count;
};

/**
 * Create an ASON namespace.
 **/
API_EXPORT ason_ns_t *
ason_ns_create(const ason_ns_ops_t *ops, const char *setup)
{
	ason_ns_t *ns = xmalloc(sizeof(ason_ns_t));

	ns->parent = NULL;
	ns->ops = ops;
	ns->data = ns->ops->init(setup);
	ns->subns_count = 0;
	ns->subns = NULL;

	return ns;
}

/**
 * Find a subspace by the given name in the given namespace.
 **/
ason_ns_t *
ason_ns_lookup_sub(const ason_ns_t *ns, const char *name)
{
	size_t i;

	for (i = 0; i < ns->subns_count; i++)
		if (! strcmp(ns->subns[i].name, name))
			break;

	if (ns->subns_count == i)
		return NULL;

	return ns->subns[i].space;
}

/**
 * Detach a subspace from its parent.
 **/
API_EXPORT void
ason_ns_detach(ason_ns_t *ns)
{
	ason_ns_t *parent = ns->parent;
	size_t i;

	for (i = 0; i < parent->subns_count; i++)
		if (parent->subns[i].space == ns)
			break;

	if (parent->subns_count == i)
		errx(1, "Namespace not listed in parent");

	parent->subns_count--;
	memcpy(&parent->subns[i + 1], &parent->subns[i],
	       (parent->subns_count - i) * sizeof(struct ason_subns));
	ns->parent = NULL;
}

/**
 * Attach a namespace to another as a subspace.
 **/
API_EXPORT ason_ns_t *
ason_ns_attach(ason_ns_t *ns, ason_ns_t *parent, const char *name)
{
	const char *test = name;
	struct ason_subns *sub;

	if (! *test)
		return NULL;

	while (*test && (isalpha(*test) || isdigit(*test) || *test == '_'))
		test++;

	if (*test)
		return NULL;

	if (ns->parent)
		ason_ns_detach(ns);

	parent->subns = xrealloc(parent->subns, (parent->subns_count + 1) *
				 sizeof(struct ason_subns));

	sub = &parent->subns[parent->subns_count++];

	ns->parent = parent;
	sub->name = xstrdup(name);
	sub->space = ns;

	return ns;
}

/**
 * Destroy an ASON namespace.
 **/
API_EXPORT void
ason_ns_destroy(ason_ns_t *ns)
{
	size_t i;

	if (ns->parent)
		ason_ns_detach(ns);

	for (i = 0; i < ns->subns_count; i++) {
		free(ns->subns[i].name);
		ason_ns_destroy(ns->subns[i].space);
	}
	
	free(ns->subns);
	ns->ops->teardown(ns->data);
	free(ns);
}


/**
 * Convert a namespace and a value of the form "foo.bar.baz" into the subspace
 * "bar" and the single name "baz."
 **/
static ason_ns_t *
ason_ns_resolve_subspaces(ason_ns_t *ns, const char **name)
{
	const char *test = *name;
	char *tmp;

	for (;;) {
		while (*test && *test != '.')
			test++;

		if (! *test)
			return ns;

		tmp = xstrndup(*name, test - *name);
		test++;
		*name = test;

		ns = ason_ns_lookup_sub(ns, tmp);

		if (! ns)
			return NULL;

		free(tmp);
	}
}

/**
 * Find a subspace of a given namespace.
 **/
API_EXPORT ason_ns_t *
ason_ns_get_sub(ason_ns_t *ns, const char *name)
{
	ns = ason_ns_resolve_subspaces(ns, &name);

	if (! ns)
		return NULL;

	return ason_ns_lookup_sub(ns, name);
}

/**
 * Load a value from an ASON namespace.
 **/
API_EXPORT ason_t *
ason_ns_load(ason_ns_t *ns, const char *name)
{
	ns = ason_ns_resolve_subspaces(ns, &name);

	if (! ns)
		return NULL;

	return ns->ops->load(ns->data, name);
}

/**
 * Store a value into an ASON namespace.
 **/
API_EXPORT int
ason_ns_store(ason_ns_t *ns, const char *name, ason_t *value)
{
	ns = ason_ns_resolve_subspaces(ns, &name);

	if (! ns)
		return -ENOENT;

	return ns->ops->store(ns->data, name, value);
}

/**
 * Create a value in an ASON namespace.
 **/
API_EXPORT int
ason_ns_mkvar(ason_ns_t *ns, const char *name)
{
	ns = ason_ns_resolve_subspaces(ns, &name);

	if (! ns)
		return -ENOENT;

	return ns->ops->mkvar(ns->data, name);
}

/**
 * Set metadata for a variable in an ASON namespace.
 **/
API_EXPORT int
ason_ns_set_meta(ason_ns_t *ns, const char *name, const char *meta)
{
	ns = ason_ns_resolve_subspaces(ns, &name);

	if (! ns)
		return -ENOENT;

	return ns->ops->set_meta(ns->data, name, meta);
}

/**
 * Get metadata for a variable in an ASON namespace.
 **/
API_EXPORT const char *
ason_ns_get_meta(ason_ns_t *ns, const char *name)
{
	ns = ason_ns_resolve_subspaces(ns, &name);

	if (! ns)
		return NULL;

	return ns->ops->get_meta(ns->data, name);
}

/**
 * Register a new type of namespace.
 **/
API_EXPORT int
ason_ns_register_proto(const ason_ns_ops_t *ops, const char *name)
{
	const char *pos = name;

	if (! *pos)
		return -EINVAL;

	while (*pos && *pos != ':')
		pos++;

	if (*pos)
		return -EINVAL;

	ason_ns_ops_registry = xrealloc(ason_ns_ops_registry,
					ason_ns_ops_registry_count + 1);

	ason_ns_ops_registry[ason_ns_ops_registry_count].name = xstrdup(name);
	ason_ns_ops_registry[ason_ns_ops_registry_count].ops = ops;
	ason_ns_ops_registry_count++;

	return 0;
}

/**
 * Create a namespace from a URL.
 **/
API_EXPORT ason_ns_t *
ason_ns_connect(const char *name)
{
	size_t i;
	const char *args = name;

	if (! ason_ns_ops_registry)
		ason_ns_register_proto(ASON_NS_RAM, "ram");

	while (*args && *args != ':')
		args++;

	for (i = 0; i < ason_ns_ops_registry_count; i++) {
		if (! strncmp(ason_ns_ops_registry[i].name, name, args - name))
			break;
	}

	if (i == ason_ns_ops_registry_count)
		return NULL;

	if (*args)
		args++;

	return ason_ns_create(ason_ns_ops_registry[i].ops, args);
}
