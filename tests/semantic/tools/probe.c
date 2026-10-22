#define _XOPEN_SOURCE 700

#include "compiler/analysis/semantics/check.h"
#include "compiler/common/host/files.h"
#include "compiler/common/host/streams.h"

#include <stdlib.h>
#include <string.h>

static void write_error(const RvCheckError *error, bool relative) {
    char *directory = relative ? rv_host_canonical_path(".") : NULL;
    const char *path = error->span.source == NULL ? error->path : rv_source_path(error->span.source);
    size_t length = directory == NULL ? 0 : strlen(directory);
    if (path != NULL && length != 0 && strncmp(path, directory, length) == 0
        && path[length] == '/') {
        path += length + 1;
        RvSourcePosition position;
        if (error->span.source != NULL
            && rv_source_position(error->span.source, error->span.start, &position)) {
            fprintf(stderr, "%s:%zu:%zu: error: %s\n", path, position.line, position.column, error->message);
        } else {
            fprintf(stderr, "%s: error: %s\n", path, error->message);
        }
    } else {
        rv_check_error_write(stderr, error);
    }
    free(directory);
}

int main(int argc, char **argv) {
    if (!rv_host_initialize_streams()) {
        return EXIT_FAILURE;
    }
    bool ast = false;
    bool summary = false;
    bool relative = false;
    int index = 1;
    while (index < argc - 1) {
        if (strcmp(argv[index], "--ast") == 0) {
            ast = true;
        } else if (strcmp(argv[index], "--summary") == 0) {
            summary = true;
        } else if (strcmp(argv[index], "--relative-diagnostics") == 0) {
            relative = true;
        } else {
            break;
        }
        ++index;
    }
    if (index != argc - 1) {
        fputs("usage: semantic-probe [--ast] [--summary] [--relative-diagnostics] source\n", stderr);
        return EXIT_FAILURE;
    }

    RvCheckedProgram program;
    RvCheckError error;
    bool success = rv_check_program(argv[index], rv_allocator_system(), &program, &error);
    if (!success) {
        write_error(&error, relative);
    } else {
        if (summary) {
            printf("modules=%zu symbols=%zu\n", program.graph.modules.length, program.symbols.length);
        }
        if (ast && !rv_ast_dump(stdout, &program.graph.entry->ast)) {
            fputs("cannot write checked AST\n", stderr);
            success = false;
        }
    }

    rv_checked_program_destroy(&program);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
