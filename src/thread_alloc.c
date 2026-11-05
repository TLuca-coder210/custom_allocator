#include "thread_alloc.h"
#include "my_alloc.c"
#include "my_alloc.h"
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>

extern void *my_alloc(size_t size);
extern void my_free(void *ptr);

#define NUM_BINS 16
#define MAX_CACHED_SIZE 256

typedef struct tcache_node{
    struct tcache_node *next;
} tcache_node_t;

typedef struct{
    tcache_node_t *bins[NUM_BINS];
} thread_cache_t;

typedef struct {
    uint32_t code;
    size_t block_size;
} slab_header_t;

static __thread thread_cache_t tcache = {0};

static void* tc_refill(int index){
    size_t size = (index + 1) * 16;
    size_t chunk_size = PAGE_SIZE;

    char *chunk = (char*)mmap(NULL, chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

    if(chunk == MAP_FAILED){
        return NULL;
        // out of memory
    }
    slab_header_t *information_block = (slab_header_t*)chunk;
    information_block->block_size = size;
    information_block->code = 0xCAFEBABE;
    size_t header_offset = 16;
    int block_count = (chunk_size - header_offset) / size;

    char *current_slice = chunk + header_offset;
    void *result = (void *)current_slice;
    current_slice += size;
    tcache.bins[index] = (tcache_node_t*)current_slice;

    for(int i = 1; i < block_count - 1; i++){
        tcache_node_t *node = (tcache_node_t*)current_slice;
        char *next_slice = current_slice + size;
        node->next = (tcache_node_t*)next_slice;
        current_slice = next_slice;
    }
    tcache_node_t *last_slice = (tcache_node_t*)current_slice;
    last_slice->next = NULL;
    return result;
}

void *tc_alloc(size_t size){
    if(size == 0){
        return NULL;
    }
    if(size > MAX_CACHED_SIZE){
        return my_alloc(size);
    }
    size_t aligned_size = (size / 16 + 1) * 16;
    if(size % 16 == 0){
        aligned_size = size;
    }
    // from here on we use aligned_size 
    int index = (aligned_size) / 16 - 1;
    if(tcache.bins[index] == NULL){
        // call my_alloc function -> linear time complexity
        return tc_refill(index);
    }
    else{
        // O(1) time complexity
        tcache_node_t *ptr = tcache.bins[index];
        tcache.bins[index] = ptr->next;
        return ptr;
    }
}

void tc_free(void *ptr){
    // for ptr the size is unknown
    if(ptr == NULL){
        return;
    }
    uintptr_t ptr_val = (uintptr_t)ptr;
    uintptr_t page_start = ptr_val & ~(PAGE_SIZE - 1);

    slab_header_t *ptr_start = (slab_header_t*)page_start;
    if(ptr_start->code != 0xCAFEBABE){
        my_free(ptr);
        return;
    }
    size_t size = ptr_start->block_size;

    int index = size / 16 - 1;
    tcache_node_t *start_stack = tcache.bins[index];
    tcache_node_t *new_start = (tcache_node_t*)ptr;
    tcache.bins[index] = new_start;
    new_start->next = start_stack;
}

void *tc_realloc(void *ptr, size_t size){
    if(ptr == NULL){
        return tc_alloc(size);
    }
    if(size == 0){
        tc_free(ptr);
        return NULL;
    }
    uintptr_t ptr_val = (uintptr_t)ptr;
    uintptr_t page_start = ptr_val & ~(PAGE_SIZE - 1);

    slab_header_t *ptr_start = (slab_header_t*)page_start;
    if(ptr_start->code != 0xCAFEBABE){
        return my_realloc(ptr, size);
    }
    size_t block_size = ptr_start->block_size;
    if(block_size >= size){
        return ptr;
    }
    void *new_ptr = tc_alloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }
    memcpy(new_ptr, ptr, block_size);
    tc_free(ptr);

    return new_ptr;
}
