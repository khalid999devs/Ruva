#ifndef RUVA_ARENA_H
#define RUVA_ARENA_H

#include "allocator.h"

typedef struct RvArenaBlock RvArenaBlock;

typedef struct {
    RvAllocator allocator;
    RvArenaBlock *head;
    size_t block_size;
} RvArena;

void rv_arena_init(RvArena *arena, RvAllocator allocator, size_t block_size);
void *rv_arena_allocate(RvArena *arena, size_t size);
char *rv_arena_copy(RvArena *arena, const void *bytes, size_t length);
void rv_arena_destroy(RvArena *arena);

#endif
