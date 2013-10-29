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

#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include <ason/namespace.h>
#include <ason/value.h>

#include "util.h"

/**
 * Entry in a RAM namespace.
 **/
struct ram_ns_entry {
	char *name;
	ason_t *value;
};

/**
 * A RAM namespace.
 **/
struct ram_ns {
	struct ram_ns_entry *entries;
	size_t count;
};

/**
 * Initialize a RAM namespace.
 **/
static void *
ason_ram_ns_init(const char *setup)
{
	struct ram_ns *ret = xmalloc(sizeof(struct ram_ns));

	(void)setup;

	ret->entries = NULL;
	ret->count = 0;

	return ret;
}

/**
 * Tear down a RAM namespace.
 **/
static void
ason_ram_ns_teardown(void *data)
{
	struct ram_ns *ns = data;
	size_t i;

	for (i = 0; i < ns->count; i++) {
		free(ns->entries[i].name);
		ason_destroy(ns->entries[i].value);
	}

	free(ns);
}

/**
 * Load a value from a RAM namespace.
 **/
static ason_t *
ason_ram_ns_load(void *data, const char *name)
{
	struct ram_ns *ns = data;
	size_t i;

	for (i = 0; i < ns->count; i++)
		if (! strcmp(ns->entries[i].name, name))
			return ason_copy(ns->entries[i].value);

	return NULL;
}

/**
 * Store a value into an ASON RAM namespace.
 **/
static int
ason_ram_ns_store(void *data, const char *name, ason_t *value)
{
	struct ram_ns *ns = data;
	size_t i;

	for (i = 0; i < ns->count; i++) {
		if (strcmp(ns->entries[i].name, name))
			continue;

		ason_destroy(ns->entries[i].value);
		ns->entries[i].value = ason_copy(value);
		return 0;
	}

	return -ENOENT;
}

/**
 * Make a new variable in an ASON RAM namespace.
 **/
static int
ason_ram_ns_mkvar(void *data, const char *name)
{
	struct ram_ns *ns = data;
	size_t i;
	const char *iter;

	if (!*name || isdigit(*name))
		return -EINVAL;

	for (iter = name; *iter; iter++) {
		if (isalpha(*iter))
			continue;
		if (isdigit(*iter))
			continue;
		if (*iter == '_')
			continue;

		return -EINVAL;
	}

	for (i = 0; i < ns->count; i++)
		if (! strcmp(ns->entries[i].name, name))
			return -EEXIST;

	ns->entries = xrealloc(ns->entries, (++ns->count) *
			       sizeof(struct ram_ns_entry));

	ns->entries[ns->count - 1].name = xstrdup(name);
	ns->entries[ns->count - 1].value = ASON_EMPTY;

	return 0;
}

/**
 * Set metadata for a variable in a RAM namespace. Not supported.
 **/
static int
ason_ram_ns_set_meta(void *data, const char *name, const char *meta)
{
	(void)data;
	(void)name;
	(void)meta;
	return -ENOTSUP;
}

/**
 * Get metadata for a variable in a RAM namespace. Not supported.
 **/
static const char *
ason_ram_ns_get_meta(void *data, const char *name)
{
	(void)data;
	(void)name;
	return "";
}

/**
 * Operations struct for a RAM namespace.
 **/
static const ason_ns_ops_t ASON_NS_RAM_DATA = {
	.init = ason_ram_ns_init,
	.teardown = ason_ram_ns_teardown,
	.load = ason_ram_ns_load,
	.store = ason_ram_ns_store,
	.mkvar = ason_ram_ns_mkvar,
	.set_meta = ason_ram_ns_set_meta,
	.get_meta = ason_ram_ns_get_meta,
};
API_EXPORT const ason_ns_ops_t *ASON_NS_RAM = &ASON_NS_RAM_DATA;
