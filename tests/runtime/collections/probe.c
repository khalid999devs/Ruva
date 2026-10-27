#include "../test.h"

#include <string.h>

bool test_runtime_array_failure(RvRuntime *runtime, TestAllocator *allocations,
                                 const char *mode, RvLocation where) {
    RvArray array = {0};
    if (strcmp(mode, "array-length") == 0) {
        array.length = (size_t)INT32_MAX + 1;
        rv_array_length(runtime, &array, where);
    } else if (strcmp(mode, "array-limit") == 0) {
        array.length = INT32_MAX;
        array.capacity = INT32_MAX;
        rv_array_push(runtime, &array, (RvElement){.integer = 1}, where);
    } else if (strcmp(mode, "array-allocation") == 0) {
        RvText *text = rv_text_create(runtime, "argument", 8, where);
        allocations->fail_at = allocations->calls + 1;
        array.owns_text = true;
        rv_array_push(runtime, &array, (RvElement){.text = text}, where);
    } else if (strcmp(mode, "array-resize") == 0) {
        rv_array_push(runtime, &array, (RvElement){.integer = 1}, where);
        while (array.length < array.capacity) {
            rv_array_push(runtime, &array, (RvElement){.integer = 2}, where);
        }
        allocations->fail_at = allocations->calls + 1;
        rv_array_push(runtime, &array, (RvElement){.integer = 3}, where);
    } else {
        return false;
    }
    rv_array_release(runtime, &array);
    return true;
}
