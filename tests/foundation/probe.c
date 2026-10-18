#include "test.h"
#include "compiler/common/memory/arena.h"
#include "compiler/common/source/source.h"
#include "compiler/common/diagnostics/diagnostic.h"

#include <stdlib.h>
#include <string.h>

static int inspect_source(const char *path, bool diagnostic) {
    RvSourceError error;
    RvSource *source = rv_source_load(rv_allocator_system(), path, &error);
    if (source == NULL) {
        rv_diagnostic_load_error(stderr, path, error);
        return EXIT_FAILURE;
    }
    if (diagnostic) {
        RvSpan span;
        if (!rv_span_create(&span, source, 15, 19)
            || !rv_diagnostic_write(stderr, span, "example diagnostic")) {
            rv_source_destroy(source);
            return EXIT_FAILURE;
        }
        rv_source_destroy(source);
        return EXIT_FAILURE;
    }
    printf("bytes=%zu lines=%zu\n", rv_source_length(source), rv_source_line_count(source));
    for (size_t line = 1; line <= rv_source_line_count(source); ++line) {
        RvSourceLine value;
        if (!rv_source_line(source, line, &value)) {
            rv_source_destroy(source);
            return EXIT_FAILURE;
        }
        printf("line=%zu bytes=%zu\n", line, value.length);
    }
    RvSourcePosition end;
    if (!rv_source_position(source, rv_source_length(source), &end)) {
        rv_source_destroy(source);
        return EXIT_FAILURE;
    }
    printf("eof=%zu:%zu\n", end.line, end.column);
    rv_source_destroy(source);
    return EXIT_SUCCESS;
}

static int copy_input(void) {
    FILE *output = fopen("result.txt", "wb");
    if (output == NULL) {
        return EXIT_FAILURE;
    }
    unsigned char buffer[256];
    size_t count;
    bool success = true;
    while ((count = fread(buffer, 1, sizeof(buffer), stdin)) != 0) {
        if (fwrite(buffer, 1, count, stdout) != count
            || fwrite(buffer, 1, count, output) != count) {
            success = false;
            break;
        }
    }
    if (ferror(stdin)) {
        success = false;
    }
    if (fclose(output) != 0) {
        success = false;
    }
    if (success) {
        fputs("copied input\n", stderr);
    }
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char **argv) {
    if (argc == 3 && (strcmp(argv[1], "inspect") == 0 || strcmp(argv[1], "diagnostic") == 0)) {
        return inspect_source(argv[2], strcmp(argv[1], "diagnostic") == 0);
    }
    if (argc == 2 && strcmp(argv[1], "copy") == 0) {
        return copy_input();
    }
    if (argc == 2 && strcmp(argv[1], "allocation-failure") == 0) {
        TestAllocator state = {.fail_at = 1};
        RvArena arena;
        rv_arena_init(&arena, test_allocator(&state), 0);
        void *value = rv_arena_allocate(&arena, 32);
        bool failed = value == NULL;
        rv_arena_destroy(&arena);
        if (failed && state.live == 0) {
            fputs("arena: allocation failed cleanly\n", stderr);
            return EXIT_FAILURE;
        }
    }
    fputs("invalid test probe invocation\n", stderr);
    return EXIT_FAILURE;
}
