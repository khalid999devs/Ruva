#ifndef RUVA_ALLOCATOR_H
#define RUVA_ALLOCATOR_H

#include "compiler/common/platform.h"

#include <stdbool.h>
#include <stddef.h>

/* Custom callbacks follow malloc/realloc/free semantics. */
typedef struct {
    void *context;
    void *(*allocate)(void *context, size_t size);
    void *(*resize)(void *context, void *pointer, size_t size);
    void (*release)(void *context, void *pointer);
} RvAllocator;

RvAllocator rv_allocator_system(void);
bool rv_size_add(size_t left, size_t right, size_t *result);
bool rv_size_multiply(size_t left, size_t right, size_t *result);

#endif
