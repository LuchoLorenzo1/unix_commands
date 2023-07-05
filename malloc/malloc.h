#ifndef _MALLOC_H_
#define _MALLOC_H_
#include <stddef.h>
#include <stdbool.h>

struct region {
	bool free;
	size_t size;
	int index;
	struct region *next;
	struct region *prev;
};

struct malloc_stats {
	int mallocs;
	int frees;
	int requested_memory;
	int leng_block_16;
	int leng_block_m1;
	int leng_block_m32;
};

void *malloc(size_t size);

void free(void *ptr);

void *calloc(size_t nmemb, size_t size);

void *realloc(void *ptr, size_t size);

void get_stats(struct malloc_stats *stats);

#endif  // _MALLOC_H_
