#include "test.h"
#include "compiler/common/memory/arena.h"
#include "compiler/common/memory/vector.h"

#include <string.h>

static bool checked_sizes(void) {
    size_t result = 17;
    CHECK(!rv_size_add(SIZE_MAX, 1, &result));
    CHECK(result == 17);
    CHECK(!rv_size_multiply(SIZE_MAX, 2, &result));
    CHECK(result == 17);
    CHECK(rv_size_add(SIZE_MAX - 2, 2, &result) && result == SIZE_MAX);
    CHECK(rv_size_multiply(SIZE_MAX, 0, &result) && result == 0);
    CHECK(rv_size_multiply(12, 13, &result) && result == 156);
    return true;
}

static bool arena_alignment_and_lifetime(void) {
    TestAllocator state = {0};
    RvArena arena;
    rv_arena_init(&arena, test_allocator(&state), 64);
    char original[] = "stable";
    char *copy = rv_arena_copy(&arena, original, sizeof(original) - 1);
    CHECK(copy != NULL);
    original[0] = 'x';
    for (size_t size = 1; size < 256; ++size) {
        void *pointer = rv_arena_allocate(&arena, size);
        CHECK(pointer != NULL);
        CHECK((uintptr_t)pointer % _Alignof(max_align_t) == 0);
        memset(pointer, 0xa5, size);
    }
    CHECK(strcmp(copy, "stable") == 0);
    char *empty = rv_arena_copy(&arena, NULL, 0);
    CHECK(empty != NULL && empty[0] == '\0');
    CHECK(rv_arena_allocate(&arena, 0) == NULL);
    CHECK(rv_arena_copy(&arena, NULL, 1) == NULL);
    CHECK(rv_arena_allocate(&arena, SIZE_MAX) == NULL);
    CHECK(rv_arena_copy(&arena, "x", SIZE_MAX) == NULL);
    rv_arena_destroy(&arena);
    CHECK(state.live == 0);
    rv_arena_destroy(&arena);
    CHECK(rv_arena_allocate(&arena, 8) != NULL);
    rv_arena_destroy(&arena);
    CHECK(state.live == 0);
    return true;
}

static bool arena_failure_preserves_existing_values(void) {
    TestAllocator state = {0};
    RvArena arena;
    rv_arena_init(&arena, test_allocator(&state), 8);
    char *saved = rv_arena_copy(&arena, "value", 5);
    CHECK(saved != NULL);
    state.fail_at = state.calls + 1;
    CHECK(rv_arena_allocate(&arena, 128) == NULL);
    CHECK(strcmp(saved, "value") == 0 && state.live == 1);
    state.fail_at = 0;
    CHECK(rv_arena_allocate(&arena, 128) != NULL);
    rv_arena_destroy(&arena);
    CHECK(state.live == 0);
    return true;
}

static bool vector_growth_and_failure(void) {
    TestAllocator state = {0};
    RvVector vector;
    CHECK(rv_vector_init(&vector, test_allocator(&state), sizeof(size_t)));
    for (size_t index = 0; index < 200; ++index) {
        size_t *slot = rv_vector_append(&vector);
        CHECK(slot != NULL);
        *slot = index * 3;
    }
    for (size_t index = 0; index < vector.length; ++index) {
        CHECK(*(size_t *)rv_vector_at(&vector, index) == index * 3);
    }
    CHECK(rv_vector_at(&vector, vector.length) == NULL);
    unsigned char *saved = vector.data;
    size_t capacity = vector.capacity;
    state.fail_at = state.calls + 1;
    CHECK(!rv_vector_reserve(&vector, capacity + 1));
    CHECK(vector.data == saved && vector.capacity == capacity && vector.length == 200);
    CHECK(*(size_t *)rv_vector_at(&vector, 199) == 597);
    CHECK(!rv_vector_reserve(&vector, SIZE_MAX));
    state.fail_at = 0;
    CHECK(rv_vector_reserve(&vector, capacity + 1));
    CHECK(*(size_t *)rv_vector_at(&vector, 199) == 597);
    rv_vector_destroy(&vector);
    rv_vector_destroy(&vector);
    CHECK(state.live == 0);
    CHECK(rv_vector_append(&vector) != NULL);
    rv_vector_destroy(&vector);
    CHECK(state.live == 0);
    CHECK(!rv_vector_init(&vector, test_allocator(&state), 0));
    CHECK(rv_vector_append(&vector) == NULL);
    rv_vector_destroy(&vector);
    return true;
}

static bool vector_append_failure_keeps_length(void) {
    TestAllocator state = {.fail_at = 1};
    RvVector vector;
    CHECK(rv_vector_init(&vector, test_allocator(&state), sizeof(size_t)));
    CHECK(rv_vector_append(&vector) == NULL);
    CHECK(vector.length == 0 && vector.data == NULL && state.live == 0);
    state.fail_at = 0;
    size_t *slot = rv_vector_append(&vector);
    CHECK(slot != NULL);
    *slot = 42;
    while (vector.length < vector.capacity) {
        slot = rv_vector_append(&vector);
        CHECK(slot != NULL);
        *slot = 7;
    }
    size_t length = vector.length;
    state.fail_at = state.calls + 1;
    CHECK(rv_vector_append(&vector) == NULL && vector.length == length);
    CHECK(*(size_t *)rv_vector_at(&vector, 0) == 42);
    rv_vector_destroy(&vector);
    CHECK(state.live == 0);
    return true;
}

bool test_memory(void) {
    return checked_sizes() && arena_alignment_and_lifetime()
        && arena_failure_preserves_existing_values() && vector_growth_and_failure()
        && vector_append_failure_keeps_length();
}
