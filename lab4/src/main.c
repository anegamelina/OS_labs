#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <time.h>

typedef struct {
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
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

void default_allocator_free(void* allocator, void* memory) {
    munmap(memory, 0);
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
        }
    }

    if (!library_handle) {
        api.allocator_create = default_allocator_create;
        api.allocator_destroy = default_allocator_destroy;
        api.allocator_alloc = default_allocator_alloc;
        api.allocator_free = default_allocator_free;
    }

    void* memory = mmap(NULL, 1024 * 1024, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    void* allocator = api.allocator_create(memory, 1024 * 1024);

    clock_t start = clock();
    for (int i = 0; i < 1000; i++) {
        void* ptr = api.allocator_alloc(allocator, 1024);
        api.allocator_free(allocator, ptr);
    }
    clock_t end = clock();
    printf("Time taken: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    api.allocator_destroy(allocator);
    munmap(memory, 1024 * 1024);

    if (library_handle) {
        dlclose(library_handle);
    }

    return 0;
}