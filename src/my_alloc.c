#include "my_alloc.h"
#include <sys/mman.h>
#include <unistd.h>

typedef struct metadata_block{
    size_t size;
    int is_free; // if is_free is 0, then the block is free
    struct metadata_block *next;
    struct metadata_block *prev;
} metadata_block_t;

static metadata_block_t *head = NULL;

#define PAGE_SIZE 16384

void *my_alloc(size_t size){
    if(size == 0){
        return NULL;
    }
    if(size % 16 != 0){
        size_t division = size / 16;
        size = (division + 1) * 16;
    }
    metadata_block_t *curr = head;
    metadata_block_t *last = NULL;
    while(curr != NULL){
        if(curr->is_free == 0 && curr->size >= size){
            curr->is_free = 1;
            return (void*)(curr + 1);
        }
        last = curr;
        curr = curr->next;
    }
    size_t total_needed = sizeof(metadata_block_t) + size;
    size_t alloc_size = ((total_needed + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    void *raw = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (raw == MAP_FAILED) {
        return NULL;
    }
    metadata_block_t *first_header = raw;
    first_header->size = size;
    first_header->is_free = 1;
    first_header->next = NULL;
    if(head == NULL){
        head = first_header;
        first_header->prev = NULL;
    }
    else if(last != NULL){
        last->next = first_header;
        first_header->prev = last;
    }
    size_t remaining_mem = alloc_size - size - sizeof(metadata_block_t);
    metadata_block_t *second_header = NULL;
    if(remaining_mem >= sizeof(metadata_block_t) + 16){
        char *payload_start = (char *)(first_header + 1);
        second_header = (metadata_block_t *)(payload_start + first_header->size);
        second_header->size = remaining_mem - sizeof(metadata_block_t);
        second_header->is_free = 0;
        second_header->prev = first_header;
        second_header->next = NULL;
        first_header->next = second_header;
    }
    return (void*)(first_header + 1);
    return NULL;
}
