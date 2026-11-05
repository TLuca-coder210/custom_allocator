#ifndef TCACHE_ALLOC_H
#define TCACHE_ALLOC_H

#include <stddef.h>

void *tc_alloc(size_t size);
void tc_free(void *ptr);
void *tc_realloc(void *ptr, size_t size);

#endif
