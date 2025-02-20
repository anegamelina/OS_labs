#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <time.h>

typedef struct AllocatorAPI {
    void* (*allocator_create)(void*, size_t);
    void (*allocator_destroy)(void*);
    void* (*allocator_alloc)(void*, size_t);
    void (*allocator_free)(void*, void*);
} AllocatorAPI;

void* default_allocator_create(void* memory, size_t size) {
    return memory;
}

void default_allocator_destroy(void* allocator) {
}

void* default_allocator_alloc(void* allocator, size_t size) {
    if (allocator) {
        return (void*)((char*)allocator + sizeof(size_t));
    }
    return NULL;
}

void default_allocator_free(void* allocator, void* memory) {
}

int main(int argc, char** argv) {
    AllocatorAPI api;
    void* library_handle = NULL;

    if (argc > 1) {
        library_handle = dlopen(argv[1], RTLD_LAZY);
        if (library_handle) {
            api.allocator_create = dlsym(library_handle, "allocator_create");
            api.allocator_destroy = dlsym(library_handle, "allocator_destroy");
            api.allocator_alloc = dlsym(library_handle, "allocator_alloc");
            api.allocator_free = dlsym(library_handle, "allocator_free");
        } else {
            fprintf(stderr, "Failed to load library: %s\n", dlerror());
        }
    } else {
        fprintf(stderr, "No library path provided. Using default allocator.\n");
    }

    if (!library_handle) {
        api.allocator_create = default_allocator_create;
        api.allocator_destroy = default_allocator_destroy;
        api.allocator_alloc = default_allocator_alloc;
        api.allocator_free = default_allocator_free;
    }

    size_t pool_size = 1024 * 1024;
    void* memory = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (memory == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    void* allocator = api.allocator_create(memory, pool_size);

    struct timespec start, end;
    double time_taken;

    // тестирование выделения памяти
    for (int i = 0; i < 3; i++) {
        size_t block_size = 1024 * (i + 1);  // размер блока увеличивается с каждой итерацией

        clock_gettime(CLOCK_MONOTONIC, &start);
        void* ptr = api.allocator_alloc(allocator, block_size);
        clock_gettime(CLOCK_MONOTONIC, &end);

        time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        printf("Allocated block: %p, size: %zu bytes, time: %.9f seconds\n", ptr, block_size, time_taken);

        clock_gettime(CLOCK_MONOTONIC, &start);
        api.allocator_free(allocator, ptr);
        clock_gettime(CLOCK_MONOTONIC, &end);

        time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        printf("Freed block: %p (id=%d, name=Object %d, value=%.2f), time: %.9f seconds\n",
               ptr, i + 1, i + 1, (double)(i + 1) * 123.45, time_taken);
    }

    api.allocator_destroy(allocator);
    munmap(memory, pool_size);

    if (library_handle) {
        dlclose(library_handle);
    }

    return 0;
}