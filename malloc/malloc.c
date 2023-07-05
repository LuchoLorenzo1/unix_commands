#include <string.h>
#include <sys/mman.h>
#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include "malloc.h"

#include "printfmt.h"

#include <errno.h>

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)

#define KIB16 2048
#define MIB1 131072
#define MIB32 MIB1 * 32

#define MIN_SIZE 100

// struct block {
// 	enum Size a;
// 	struct region *region;
// 	struct block *next;
// };


struct region *region_free_list = NULL;

struct region *KIB16_BLOCKS[5000];
struct region *MIB1_BLOCKS[1000];
struct region *MIB32_BLOCKS[500];

struct region **BLOCKS_LISTS[3] = { KIB16_BLOCKS, MIB1_BLOCKS, MIB32_BLOCKS };

int LEN_KIB16_BLOCKS = 0;
int LEN_MIB1_BLOCKS = 0;
int LEN_MIB32_BLOCKS = 0;

int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;

// finds the next free region
// that holds the requested size
static struct region *
find_free_region(size_t size)
{
	int n;
#ifdef FIRST_FIT
	for (int j = 0; j < 2; j++) {
		if (j == 0)
			n = LEN_KIB16_BLOCKS;
		else if (j == 1)
			n = LEN_MIB1_BLOCKS;
		else
			n = LEN_MIB32_BLOCKS;
		for (size_t i = 0; i < n; i++) {
			struct region *next = BLOCKS_LISTS[j][i];
			// Your code here for "first fit"
			while (next != NULL) {
				if (next->free && next->size >= size) {
					return next;
				}
				next = next->next;
			}
		}
	}
	return NULL;
#endif

#ifdef BEST_FIT
	struct region *minor = NULL;
	for (int j = 0; j < 2; j++) {
		if (j == 0)
			n = LEN_KIB16_BLOCKS;
		else if (j == 1)
			n = LEN_MIB1_BLOCKS;
		else
			n = LEN_MIB32_BLOCKS;
		for (size_t i = 0; i < n; i++) {
			struct region *next = BLOCKS_LISTS[j][i];
			while (next != NULL) {
				if (next->free && next->size == size)
					return next;
				if (next->free && next->size >= size) {
					if (minor == NULL)
						minor = next;
					else if (next->size < minor->size)
						minor = next;
				}
				next = next->next;
			}
		}
	}
	return minor;
#endif
}

static struct region *
grow_heap(size_t size)
{
	size_t block_size;
	if (size <= KIB16) {
		block_size = KIB16;
	} else if (size <= MIB1) {
		block_size = MIB1;
	} else if (size <= MIB32) {
		block_size = MIB32;
	} else {
		return NULL;
	}

	// finds the current heap break
	// struct region *a = (struct region *) sbrk(0);

	// allocates the requested size
	struct region *curr =
	        (struct region *) mmap(NULL,
	                               sizeof(struct region) + block_size,
	                               PROT_READ | PROT_WRITE,
	                               MAP_PRIVATE | MAP_ANONYMOUS,
	                               0,
	                               0);

	if (curr == MAP_FAILED) {
		errno = ENOMEM;
		return NULL;
	}

	// verifies that the returned address
	// is the same that the previous break
	// (ref: sbrk(2))
	// assert(curr == prev);

	// verifies that the allocation
	// is successful
	//
	// (ref: sbrk(2))

	if (curr == (struct region *) -1) {
		return NULL;
	}

	curr->size = block_size;
	curr->next = NULL;
	curr->prev = NULL;
	curr->free = false;

	switch (block_size) {
	case KIB16: {
		KIB16_BLOCKS[LEN_KIB16_BLOCKS] = curr;
		curr->index = LEN_KIB16_BLOCKS;
		LEN_KIB16_BLOCKS++;
		break;
	}
	case MIB1: {
		MIB1_BLOCKS[LEN_MIB1_BLOCKS] = curr;
		curr->index = LEN_MIB1_BLOCKS;
		LEN_MIB1_BLOCKS++;
		break;
	}
	case MIB32: {
		MIB32_BLOCKS[LEN_MIB32_BLOCKS] = curr;
		curr->index = LEN_MIB32_BLOCKS;
		LEN_MIB32_BLOCKS++;
		break;
	}
	}

	return curr;
}

/// Public API of malloc library ///

void *
malloc(size_t size)
{
	if (size < MIN_SIZE) {
		size = MIN_SIZE;
	}
	struct region *next;

	// aligns to multiple of 4 bytes
	size = ALIGN4(size);

	// updates statistics
	amount_of_mallocs++;
	requested_memory += size;

	next = find_free_region(size);

	if (!next) {
		next = grow_heap(size);
	}

	// if (next->size == size) {
	// 	next->free = false;
	// 	return REGION2PTR(next);
	// }

	if (next->size - size < sizeof(struct region) + MIN_SIZE) {
		next->free = false;
		return REGION2PTR(next);
	}

	// splitting
	struct region *remaining_free_region = ((void *) REGION2PTR(next)) + size;
	remaining_free_region->next = next->next;
	remaining_free_region->free = true;
	remaining_free_region->size = next->size - (size + sizeof(struct region));
	remaining_free_region->prev = next;
	remaining_free_region->index = next->index;

	next->size = size;
	next->free = false;
	next->next = remaining_free_region;

	return REGION2PTR(next);
}

void
free(void *ptr)
{
	if (!ptr) {
		return;
	}

	struct region *curr = PTR2REGION(ptr);
	if (!curr || curr->free == 1) {
		return;
	}
	assert(curr != NULL);
	assert(curr->free == 0);

	amount_of_frees++;
	curr->free = true;

	if (curr->next && curr->next->free) {
		curr->size += curr->next->size + sizeof(struct region);
		curr->next = curr->next->next;
		if (curr->next) {
			curr->next->prev = curr;
		}
	}

	if (curr->prev && curr->prev->free) {
		curr->prev->size += curr->size + sizeof(struct region);
		curr->prev->next = curr->next;
		if (curr->next != NULL) {
			curr->next->prev = curr->prev;
		}
		curr = curr->prev;
	}

	if (curr->prev == NULL && curr->next == NULL) {
		struct region *act;
		if (curr->size == KIB16) {
			KIB16_BLOCKS[curr->index] =
			        KIB16_BLOCKS[LEN_KIB16_BLOCKS - 1];
			LEN_KIB16_BLOCKS--;

			act = KIB16_BLOCKS[curr->index];
		} else if (curr->size == MIB1) {
			MIB1_BLOCKS[curr->index] =
			        MIB1_BLOCKS[LEN_MIB1_BLOCKS - 1];
			LEN_MIB1_BLOCKS--;

			act = MIB1_BLOCKS[curr->index];
		} else if (curr->size == MIB32) {
			MIB32_BLOCKS[curr->index] =
			        MIB32_BLOCKS[LEN_MIB32_BLOCKS - 1];
			LEN_MIB32_BLOCKS--;

			act = MIB32_BLOCKS[curr->index];
		}

		while (act != NULL) {
			act->index = curr->index;
			act = act->next;
		}
		if (munmap(curr, curr->size + sizeof(struct region)) < 0) {
			errno = ENOMEM;
			return;
		};
	}
}

void *
calloc(size_t nmemb, size_t size)
{
	if (nmemb == 0 || size == 0)
		return NULL;

	void *ptr = malloc(nmemb * size);
	if (ptr != NULL)
		memset(ptr, 0, nmemb * size);
	return ptr;
}

void *
realloc(void *ptr, size_t size)
{
	struct region *curr = PTR2REGION(ptr);
	if (curr->size >= size) {
		// Si lo que sobra puede ser una nueva region hacemos split
		if (curr->size - size > sizeof(struct region) + MIN_SIZE) {
			// splitting
			struct region *remaining_free_region =
			        ((void *) REGION2PTR(curr)) + size;
			remaining_free_region->next = curr->next;
			remaining_free_region->free = true;
			remaining_free_region->size =
			        curr->size - (size + sizeof(struct region));
			remaining_free_region->prev = curr;
			remaining_free_region->index = curr->index;

			curr->next = remaining_free_region;
		}
		curr->size = size;
		return ptr;
	}

	// coalescing with next if it's free
	if (curr->next && curr->next->free &&
	    curr->next->size + curr->size >= size) {
		if (curr->next->next) {
			curr->next->next->prev = curr;
		}
		curr->next = curr->next->next;
		return ptr;
	}

	void *res = malloc(size);
	if (res == NULL) {
		errno = ENOMEM;  // Ya se chequea en malloc igual asi que capaz no hace falta
		return NULL;
	}

	memcpy(res, ptr, curr->size);

	free(ptr);

	return res;
}

void
get_stats(struct malloc_stats *stats)
{
	stats->mallocs = amount_of_mallocs;
	stats->frees = amount_of_frees;
	stats->requested_memory = requested_memory;

	stats->leng_block_16 = LEN_KIB16_BLOCKS;
	stats->leng_block_m1 = LEN_MIB1_BLOCKS;
	stats->leng_block_m32 = LEN_MIB32_BLOCKS;
}
