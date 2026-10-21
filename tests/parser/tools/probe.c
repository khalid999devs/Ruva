#include "compiler/frontend/parser/parser.h"
#include "compiler/common/diagnostics/diagnostic.h"
#include "compiler/common/host/streams.h"

#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (!rv_host_initialize_streams()) {
        return EXIT_FAILURE;
    }
    bool entry = true;
    bool check_only = false;
    int index = 1;

    while (index < argc - 1) {
        if (strcmp(argv[index], "--module") == 0) {
            entry = false;
        } else if (strcmp(argv[index], "--check") == 0) {
            check_only = true;
        } else {
            break;
        }
        ++index;
    }

    if (index != argc - 1) {
        fputs("usage: parser-probe [--module] [--check] source\n", stderr);
        return EXIT_FAILURE;
    }

    const char *path = argv[index];
    RvSourceError source_error;
    RvSource *source = rv_source_load(rv_allocator_system(), path, &source_error);
    if (source == NULL) {
        rv_diagnostic_load_error(stderr, path, source_error);
        return EXIT_FAILURE;
    }

    RvTokenStream tokens;
    RvLexError lex_error;
    if (!rv_lex_source(source, rv_allocator_system(), &tokens, &lex_error)) {
        rv_diagnostic_write(stderr, lex_error.span, lex_error.message);
        rv_source_destroy(source);
        return EXIT_FAILURE;
    }

    RvAstModule module;
    RvParseError parse_error;
    bool success = rv_parse_module(&tokens, entry, rv_allocator_system(), &module, &parse_error);
    rv_token_stream_destroy(&tokens);

    if (!success) {
        rv_diagnostic_write(stderr, parse_error.span, parse_error.message);
    } else if (!check_only && !rv_ast_dump(stdout, &module)) {
        fputs("cannot write AST output\n", stderr);
        success = false;
    }

    rv_ast_module_destroy(&module);
    rv_source_destroy(source);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
