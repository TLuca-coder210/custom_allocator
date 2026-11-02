
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

// allocation logic
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

// free memory logic
void my_free(void *ptr){
    if(ptr == NULL){
        return;
    }
    metadata_block_t *block = ((metadata_block_t*)ptr) - 1;
    if(block == NULL){
        return;
    }
    block->is_free = 0;
    // coalescing the right blocks
    if(block->next != NULL && block->next->is_free == 0){
        metadata_block_t *next_block = block->next;
        block->size += sizeof(metadata_block_t) + next_block->size;
        block->next = next_block->next;
        if(next_block->next != NULL){
            next_block->next->prev = block;
        }
    }
    // coalescing the left blocks
    if(block->prev != NULL && block->prev->is_free == 0){
        metadata_block_t *prev_block = block->prev;
        prev_block->size += sizeof(metadata_block_t) + block->size;
        prev_block->next = block->next;
        if(block->next != NULL){
            block->next->prev = prev_block;
        }
    }
}

void *my_realloc(void *ptr, size_t size){
    if(ptr == NULL){
        return my_alloc(size);
    }
    if(size == 0){
        my_free(ptr);
        return NULL;
    }
    size_t aligned_size = 0;
    if(size % 16 == 0){
        aligned_size = size;
    }
    else{
        aligned_size = (size / 16 + 1) * 16;
    }
    metadata_block_t *block = ((metadata_block_t*)ptr) - 1;
    if(block->size == aligned_size){
        return ptr;
    }   
    else if(block->size >= aligned_size){
        size_t total_size = block->size;
        block->size = aligned_size;
        if(total_size - block->size >= sizeof(metadata_block_t) + 16){
            // create a new block and mark it as free
            metadata_block_t *new_block = (void*)(ptr + aligned_size);
            new_block->size = total_size - block->size - sizeof(metadata_block_t);
            new_block->is_free = 1;
            new_block->prev = block;
            if(block->next != NULL){
                new_block->next = block->next;
                block->next->prev = new_block;
                block->next = new_block;
            }
            my_free(new_block);
        }
        return ptr;
    }
    else{
        if(block->next != NULL && block->next->is_free == 0){
            size_t combined_size = block->size + sizeof(metadata_block_t) + block->next->size;
            if(combined_size >= aligned_size){
                block->next = block->next->next;
                if(block->next != NULL){
                    block->next->prev = block;
                }
                block->size = combined_size;
                return ptr;
            }
        }
        void *new_ptr = my_alloc(aligned_size);
        if(new_ptr == NULL){
            return NULL;
        }
        memcpy(new_ptr, ptr, block->size);
        my_free(ptr);
        return new_ptr;
    }
}
