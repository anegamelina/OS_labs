#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

typedef struct Block {
    size_t size;
    struct Block* next;
} Block;

typedef struct {
    void* memory;
    size_t size;
    Block* free_list;
} Allocator;

Allocator* allocator_create(void* memory, size_t size) {
    Allocator* allocator = (Allocator*)memory;
    allocator->memory = (char*)memory + sizeof(Allocator);
    allocator->size = size;
    allocator->free_list = (Block*)allocator->memory;

    allocator->free_list->size = size - sizeof(Allocator);
    allocator->free_list->next = NULL;

    return allocator;
}

void allocator_destroy(Allocator* allocator) {
    munmap(allocator, allocator->size);
}

void* allocator_alloc(Allocator* allocator, size_t size) {
    Block* best_fit = NULL;
    Block* prev = NULL;
    Block* curr = allocator->free_list;

    while (curr != NULL) {
        if (curr->size >= size) {
            if (best_fit == NULL || curr->size < best_fit->size) {
                best_fit = curr;
                prev = curr;
            }
        }
        curr = curr->next;
    }

    if (best_fit != NULL) {
        if (best_fit->size > size + sizeof(Block)) {
            Block* new_block = (Block*)((char*)best_fit + sizeof(Block) + size);
            new_block->size = best_fit->size - size - sizeof(Block);
            new_block->next = best_fit->next;
            best_fit->size = size;
            best_fit->next = new_block;
        }

        if (prev == NULL) {
            allocator->free_list = best_fit->next;
        } else {
            prev->next = best_fit->next;
        }

        return (void*)((char*)best_fit + sizeof(Block));
    }
    return NULL;
}

void allocator_free(Allocator* allocator, void* memory) {
    Block* block = (Block*)((char*)memory - sizeof(Block));
    block->next = allocator->free_list;
    allocator->free_list = block;
}