#include "test.h"

#include <stdlib.h>
#include <string.h>

static TestAllocator allocations;

static void verify_cleanup(void) {
    if (allocations.live != 0) {
        fputs("runtime cleanup leaked\n", stderr);
        _Exit(99);
    }
}

int main(int argc, char **argv) {
    if (argc != 2 || atexit(verify_cleanup) != 0) {
        return EXIT_FAILURE;
    }
    RvRuntime runtime;
    rv_runtime_init(&runtime, stdout, stderr);
    RvAllocator allocator = test_allocator(&allocations);
    runtime.allocator = (RvRuntimeAllocator){
        allocator.context, allocator.allocate, allocator.resize, allocator.release
    };
    RvLocation where = {"panic.ruv", 3, 7};
    rv_text_create(&runtime, "live during panic", 17, where);
    if (strcmp(argv[1], "add") == 0) {
        rv_int_add(&runtime, INT32_MAX, 1, where);
    } else if (strcmp(argv[1], "subtract") == 0) {
        rv_int_subtract(&runtime, INT32_MIN, 1, where);
    } else if (strcmp(argv[1], "multiply") == 0) {
        rv_int_multiply(&runtime, INT32_MAX, 2, where);
    } else if (strcmp(argv[1], "divide") == 0) {
        rv_int_divide(&runtime, INT32_MIN, -1, where);
    } else if (strcmp(argv[1], "negate") == 0) {
        rv_int_negate(&runtime, INT32_MIN, where);
    } else if (strcmp(argv[1], "zero") == 0) {
        rv_int_divide(&runtime, 1, 0, where);
    } else if (strcmp(argv[1], "remainder") == 0) {
        rv_int_remainder(&runtime, 1, 0, where);
    } else if (strcmp(argv[1], "real-zero") == 0) {
        rv_real_divide(&runtime, 1.0, -0.0, where);
    } else if (strcmp(argv[1], "allocation") == 0) {
        allocations.fail_at = allocations.calls + 1;
        rv_allocate(&runtime, 16, where);
    } else if (strcmp(argv[1], "utf8") == 0) {
        rv_text_create(&runtime, "\xed\xa0\x80", 3, where);
    } else if (strcmp(argv[1], "nul") == 0) {
        rv_text_create(&runtime, "a\0b", 3, where);
    } else if (strcmp(argv[1], "overlong") == 0) {
        rv_text_create(&runtime, "\xc0\xaf", 2, where);
    } else if (strcmp(argv[1], "truncated") == 0) {
        rv_text_create(&runtime, "\xe2\x82", 2, where);
    } else if (strcmp(argv[1], "too-high") == 0) {
        rv_text_create(&runtime, "\xf4\x90\x80\x80", 4, where);
    } else if (strcmp(argv[1], "continuation") == 0) {
        rv_text_create(&runtime, "\x80", 1, where);
    } else if (strcmp(argv[1], "format") == 0) {
        char buffer[1];
        rv_format_real(&runtime, buffer, sizeof(buffer), 1.25, where);
    } else if (test_runtime_array_failure(&runtime, &allocations, argv[1], where)) {
        fputs("expected array panic did not occur\n", stderr);
    } else if (test_runtime_io_failure(&runtime, argv[1], where)) {
        fputs("expected I/O failure did not occur\n", stderr);
    } else if (test_runtime_text_failure(&runtime, argv[1], where)) {
        fputs("expected text failure did not occur\n", stderr);
    } else {
        rv_runtime_cleanup(&runtime);
        return EXIT_FAILURE;
    }
    rv_runtime_cleanup(&runtime);
    return EXIT_SUCCESS;
}
