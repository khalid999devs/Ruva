#include "../ruva_runtime.h"

#include <stdlib.h>

union RvAllocation {
    max_align_t alignment;
    struct {
        RvAllocation *previous;
        RvAllocation *next;
        size_t size;
    } item;
};

void *rv_try_allocate(RvRuntime *runtime, size_t size) {
    if (size == 0 || size > SIZE_MAX - sizeof(RvAllocation)
        || size > SIZE_MAX - runtime->live_bytes || runtime->live_allocations == SIZE_MAX) {
        return NULL;
    }
    RvAllocation *allocation = runtime->allocator.allocate(
        runtime->allocator.context, sizeof(*allocation) + size
    );
    if (allocation == NULL) {
        return NULL;
    }
    allocation->item.previous = NULL;
    allocation->item.next = runtime->allocations;
    allocation->item.size = size;
    if (runtime->allocations != NULL) {
        runtime->allocations->item.previous = allocation;
    }
    runtime->allocations = allocation;
    ++runtime->live_allocations;
    runtime->live_bytes += size;
    return allocation + 1;
}

void *rv_try_resize(RvRuntime *runtime, void *pointer, size_t size) {
    if (pointer == NULL) {
        return rv_try_allocate(runtime, size);
    }
    if (size == 0 || size > SIZE_MAX - sizeof(RvAllocation)) {
        return NULL;
    }
    RvAllocation *allocation = (RvAllocation *)pointer - 1;
    size_t remaining = runtime->live_bytes - allocation->item.size;
    if (size > SIZE_MAX - remaining) {
        return NULL;
    }
    RvAllocation *previous = allocation->item.previous;
    RvAllocation *next = allocation->item.next;
    RvAllocation *resized = runtime->allocator.resize(
        runtime->allocator.context, allocation, sizeof(*allocation) + size
    );
    if (resized == NULL) {
        return NULL;
    }
    resized->item.size = size;
    if (previous == NULL) {
        runtime->allocations = resized;
    } else {
        previous->item.next = resized;
    }
    if (next != NULL) {
        next->item.previous = resized;
    }
    runtime->live_bytes = remaining + size;
    return resized + 1;
}

void *rv_allocate(RvRuntime *runtime, size_t size, RvLocation where) {
    void *pointer = rv_try_allocate(runtime, size);
    if (pointer == NULL) {
        rv_panic(runtime, "out of memory", where);
    }
    return pointer;
}

void rv_release(RvRuntime *runtime, void *pointer) {
    if (pointer == NULL) {
        return;
    }
    RvAllocation *allocation = (RvAllocation *)pointer - 1;
    if (allocation->item.previous == NULL) {
        runtime->allocations = allocation->item.next;
    } else {
        allocation->item.previous->item.next = allocation->item.next;
    }
    if (allocation->item.next != NULL) {
        allocation->item.next->item.previous = allocation->item.previous;
    }
    --runtime->live_allocations;
    runtime->live_bytes -= allocation->item.size;
    runtime->allocator.release(runtime->allocator.context, allocation);
}

bool rv_runtime_cleanup(RvRuntime *runtime) {
    bool success = rv_files_cleanup(runtime);
    while (runtime->allocations != NULL) {
        rv_release(runtime, runtime->allocations + 1);
    }
    return success;
}
