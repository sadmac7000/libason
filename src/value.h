
#ifndef ASON_LOCAL_VALUE_H
#define ASON_LOCAL_VALUE_H

/**
 * A Key-value pair.
 **/
struct kv_pair {
	char *key;
	ason_t *value;
};

/**
 * Data making up a value.
 **/
struct ason {
	ason_type_t type;
	union {
		int64_t n;
		uint64_t u;
		ason_t **items;
		struct kv_pair *kvs;
	};

	size_t count;
	size_t refcount;
};

/**
 * Test if an ASON value is an object.
 **/
#define IS_OBJECT(_x) (_x->type == ASON_OBJECT || _x->type == ASON_UOBJECT)
#define IS_NULL(_x) (_x->type == ASON_NULL || _x->type == ASON_STRONG_NULL)

#ifdef __cplusplus
extern "C" {
#endif

static inline void *xmalloc(size_t sz)
{
	void *ret = malloc(sz);

	if (! ret)
		errx(1, "Malloc failed");

	return ret;
}

static inline void *xcalloc(size_t memb, size_t sz)
{
	void *ret = calloc(memb, sz);

	if (! ret)
		errx(1, "Malloc failed");

	return ret;
}

static inline void *xstrdup(const char *str)
{
	void *ret = strdup(str);

	if (! ret)
		errx(1, "Malloc failed");

	return ret;
}

#ifdef __cplusplus
}
#endif

#endif /* ASON_LOCAL_VALUE_H */
