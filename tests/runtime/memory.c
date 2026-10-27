#include "test.h"

#include <stdlib.h>
#include <string.h>

typedef union {
    max_align_t alignment;
    size_t size;
} MovingBlock;

typedef struct {
    size_t live;
    size_t moves;
} MovingAllocator;

static void *moving_allocate(void *context, size_t size) {
    MovingAllocator *state = context;
    MovingBlock *block = malloc(sizeof(*block) + size);
    if (block == NULL) {
        return NULL;
    }
    block->size = size;
    ++state->live;
    return block + 1;
}

static void moving_release(void *context, void *pointer) {
    MovingAllocator *state = context;
    --state->live;
    free((MovingBlock *)pointer - 1);
}

static void *moving_resize(void *context, void *pointer, size_t size) {
    MovingAllocator *state = context;
    MovingBlock *block = (MovingBlock *)pointer - 1;
    void *replacement = moving_allocate(context, size);
    if (replacement != NULL) {
        memcpy(replacement, pointer, block->size < size ? block->size : size);
        moving_release(context, pointer);
        ++state->moves;
    }
    return replacement;
}

bool test_runtime_memory(void) {
    RvRuntime runtime;
    rv_runtime_init(&runtime, stdout, stderr);
    TestAllocator state = {0};
    RvAllocator allocator = test_allocator(&state);
    runtime.allocator = (RvRuntimeAllocator){
        allocator.context, allocator.allocate, allocator.resize, allocator.release
    };
    unsigned char *first = rv_try_allocate(&runtime, 32);
    unsigned char *middle = rv_try_allocate(&runtime, 32);
    void *last = rv_try_allocate(&runtime, 32);
    CHECK(first != NULL && middle != NULL && last != NULL);
    CHECK((uintptr_t)first % _Alignof(max_align_t) == 0);
    CHECK(runtime.live_allocations == 3 && runtime.live_bytes == 96);
    CHECK(rv_try_resize(&runtime, first, 0) == NULL);
    CHECK(rv_try_resize(&runtime, first, SIZE_MAX) == NULL);
    memset(middle, 0x5a, 32);
    state.fail_at = state.calls + 1;
    CHECK(rv_try_resize(&runtime, middle, 64) == NULL);
    CHECK(runtime.live_allocations == 3 && runtime.live_bytes == 96 && middle[31] == 0x5a);
    state.fail_at = 0;
    unsigned char *resized = rv_try_resize(&runtime, middle, 65536);
    CHECK(resized != NULL && resized[0] == 0x5a && resized[31] == 0x5a);
    CHECK(runtime.live_bytes == 65600);
    rv_release(&runtime, first);
    rv_release(&runtime, last);
    CHECK(runtime.live_allocations == 1);
    rv_release(&runtime, resized);
    CHECK(state.live == 0 && runtime.live_bytes == 0);
    CHECK(rv_try_allocate(&runtime, 0) == NULL);
    CHECK(rv_try_allocate(&runtime, SIZE_MAX) == NULL);
    CHECK(rv_try_resize(&runtime, NULL, SIZE_MAX) == NULL);
    CHECK(rv_try_allocate(&runtime, 8) != NULL);
    rv_runtime_cleanup(&runtime);
    rv_runtime_cleanup(&runtime);
    CHECK(state.live == 0 && runtime.live_allocations == 0);
    MovingAllocator moving = {0};
    runtime.allocator = (RvRuntimeAllocator){&moving, moving_allocate, moving_resize, moving_release};
    void *values[3];
    for (size_t index = 0; index < 3; ++index) {
        values[index] = rv_try_allocate(&runtime, 32);
        CHECK(values[index] != NULL);
        memset(values[index], (int)index, 32);
    }
    for (size_t index = 0; index < 3; ++index) {
        values[index] = rv_try_resize(&runtime, values[index], 64);
        CHECK(values[index] != NULL);
        CHECK(((unsigned char *)values[index])[31] == index);
    }
    CHECK(moving.moves == 3 && moving.live == 3);
    rv_release(&runtime, values[1]);
    rv_runtime_cleanup(&runtime);
    CHECK(moving.live == 0 && runtime.live_bytes == 0);
    return true;
}
