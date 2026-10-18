#ifndef RUVA_VECTOR_H
#define RUVA_VECTOR_H

#include "allocator.h"

typedef struct {
    RvAllocator allocator;
    unsigned char *data;
    size_t length;
    size_t capacity;
    size_t element_size;
} RvVector;

bool rv_vector_init(RvVector *vector, RvAllocator allocator, size_t element_size);
bool rv_vector_reserve(RvVector *vector, size_t minimum);
/* Reserve/append can invalidate pointers returned by at/append. */
void *rv_vector_append(RvVector *vector);
void *rv_vector_at(const RvVector *vector, size_t index);
void rv_vector_destroy(RvVector *vector);

#endif
