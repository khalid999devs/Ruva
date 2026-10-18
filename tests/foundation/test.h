#ifndef RUVA_FOUNDATION_TEST_H
#define RUVA_FOUNDATION_TEST_H

#include "compiler/common/memory/allocator.h"

#include <stdio.h>

#define CHECK(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expression); \
        return false; \
    } \
} while (0)

typedef struct {
    size_t calls;
    size_t live;
    size_t fail_at;
} TestAllocator;

RvAllocator test_allocator(TestAllocator *state);
bool test_stream_equals(FILE *stream, const char *expected);
bool test_memory(void);
bool test_source(void);

#endif
