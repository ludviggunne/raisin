#ifndef MEM_H_INCLUDED
#define MEM_H_INCLUDED

#include <stdlib.h>

static inline void *_malloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL)
		abort();
	return ptr;
}

static inline void *_calloc(size_t nmemb, size_t size)
{
	void *ptr = calloc(nmemb, size);
	if (ptr == NULL)
		abort();
	return ptr;
}

static inline void *_realloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	if (ptr == NULL)
		abort();
	return ptr;
}

#define malloc _malloc
#define calloc _calloc
#define realloc _realloc

#endif				/* MEM_H_INCLUDED */
