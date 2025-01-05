#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define ALIGN_SIZE(size, alignment) (((size) + (alignment - 1)) & ~(alignment - 1))
#define FREE_LIST_ALIGNMENT 8

typedef struct Block {
    size_t size;
    struct Block* next;
} Block;

typedef struct Allocator{
    void* memory;
    size_t size;
    Block* free_list;
} Allocator;

Allocator* allocator_create(void* memory, size_t size) {
    if (memory == NULL || size < sizeof(Allocator)) {
        return NULL;
    }

    Allocator* allocator = (Allocator*)memory;
    allocator->memory = (char*)memory + sizeof(Allocator);
    allocator->size = size - sizeof(Allocator);
    allocator->free_list = (Block*)allocator->memory;

    if (allocator->free_list != NULL) {
        allocator->free_list->size = allocator->size;
        allocator->free_list->next = NULL;
    }

    return allocator;
}

void allocator_destroy(Allocator* allocator) {
    if (allocator == NULL) {
        return;
    }

    allocator->memory = NULL;
    allocator->size = 0;
    allocator->free_list = NULL;
}

void* allocator_alloc(Allocator* allocator, size_t size) {
    if (allocator == NULL || size == 0) {
        return NULL;
    }

    size_t aligned_size = ALIGN_SIZE(size, FREE_LIST_ALIGNMENT);
    Block* prev = NULL;
    Block* curr = allocator->free_list;

    while (curr != NULL) {
        if (curr->size >= aligned_size) {
            if (prev != NULL) {
                prev->next = curr->next;
            } else {
                allocator->free_list = curr->next;
            }
            return (void*)((char*)curr + sizeof(Block));
        }

        prev = curr;
        curr = curr->next;
    }

    return NULL;
}

void allocator_free(Allocator* allocator, void* memory) {
    if (allocator == NULL || memory == NULL) {
        return;
    }

    Block* block = (Block*)((char*)memory - sizeof(Block));
    block->next = allocator->free_list;
    allocator->free_list = block;
}