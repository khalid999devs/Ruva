#include "test.h"

#include <stdlib.h>

static void *test_allocate(void *context, size_t size) {
    TestAllocator *state = context;
    if (++state->calls == state->fail_at) {
        return NULL;
    }
    void *pointer = malloc(size);
    if (pointer != NULL) {
        ++state->live;
    }
    return pointer;
}

static void *test_resize(void *context, void *pointer, size_t size) {
    TestAllocator *state = context;
    if (++state->calls == state->fail_at) {
        return NULL;
    }
    bool fresh = pointer == NULL;
    void *resized = realloc(pointer, size);
    if (resized != NULL && fresh) {
        ++state->live;
    }
    return resized;
}

static void test_release(void *context, void *pointer) {
    TestAllocator *state = context;
    if (pointer != NULL) {
        --state->live;
        free(pointer);
    }
}

RvAllocator test_allocator(TestAllocator *state) {
    return (RvAllocator){state, test_allocate, test_resize, test_release};
}

bool test_stream_equals(FILE *stream, const char *expected) {
    CHECK(fseek(stream, 0, SEEK_SET) == 0);
    for (const unsigned char *byte = (const unsigned char *)expected; *byte; ++byte) {
        CHECK(fgetc(stream) == *byte);
    }
    CHECK(fgetc(stream) == EOF);
    CHECK(!ferror(stream));
    return true;
}
