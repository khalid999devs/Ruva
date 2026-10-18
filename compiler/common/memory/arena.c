#include "arena.h"

#include <string.h>

struct RvArenaBlock {
    RvArenaBlock *next;
    size_t used;
    size_t capacity;
    max_align_t alignment;
    unsigned char bytes[];
};

_Static_assert(offsetof(RvArenaBlock, bytes) % _Alignof(max_align_t) == 0,
               "Arena storage must have fundamental alignment");

void rv_arena_init(RvArena *arena, RvAllocator allocator, size_t block_size) {
    *arena = (RvArena){allocator, NULL, block_size ? block_size : 4096};
}

void *rv_arena_allocate(RvArena *arena, size_t size) {
    if (size == 0) {
        return NULL;
    }

    RvArenaBlock *block = arena->head;
    if (block != NULL) {
        size_t alignment = _Alignof(max_align_t);
        size_t padding = (alignment - block->used % alignment) % alignment;
        size_t offset;
        if (rv_size_add(block->used, padding, &offset)
            && offset <= block->capacity && size <= block->capacity - offset) {
            block->used = offset + size;
            return block->bytes + offset;
        }
    }

    size_t capacity = size > arena->block_size ? size : arena->block_size;
    size_t allocation_size;
    if (!rv_size_add(sizeof(*block), capacity, &allocation_size)) {
        return NULL;
    }
    block = arena->allocator.allocate(arena->allocator.context, allocation_size);
    if (block == NULL) {
        return NULL;
    }
    block->next = arena->head;
    block->used = size;
    block->capacity = capacity;
    arena->head = block;
    return block->bytes;
}

char *rv_arena_copy(RvArena *arena, const void *bytes, size_t length) {
    size_t size;
    if ((bytes == NULL && length != 0) || !rv_size_add(length, 1, &size)) {
        return NULL;
    }
    char *copy = rv_arena_allocate(arena, size);
    if (copy != NULL) {
        if (length != 0) {
            memcpy(copy, bytes, length);
        }
        copy[length] = '\0';
    }
    return copy;
}

void rv_arena_destroy(RvArena *arena) {
    RvArenaBlock *block = arena->head;
    while (block != NULL) {
        RvArenaBlock *next = block->next;
        arena->allocator.release(arena->allocator.context, block);
        block = next;
    }
    arena->head = NULL;
}
