#include "allocator.h"

#include <stdlib.h>

static void *system_allocate(void *context, size_t size) {
    (void)context;
    return malloc(size);
}

static void *system_resize(void *context, void *pointer, size_t size) {
    (void)context;
    return realloc(pointer, size);
}

static void system_release(void *context, void *pointer) {
    (void)context;
    free(pointer);
}

RvAllocator rv_allocator_system(void) {
    return (RvAllocator){NULL, system_allocate, system_resize, system_release};
}

bool rv_size_add(size_t left, size_t right, size_t *result) {
    if (right > SIZE_MAX - left) {
        return false;
    }
    *result = left + right;
    return true;
}

bool rv_size_multiply(size_t left, size_t right, size_t *result) {
    if (left != 0 && right > SIZE_MAX / left) {
        return false;
    }
    *result = left * right;
    return true;
}
