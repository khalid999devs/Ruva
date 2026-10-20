#include "compiler/frontend/lexer/lexer.h"
#include "compiler/common/diagnostics/diagnostic.h"
#include "compiler/common/host/streams.h"

#include <stdlib.h>
#include <string.h>

static void print_bytes(const char *bytes, size_t length) {
    putchar('"');
    for (size_t index = 0; index < length; ++index) {
        unsigned char byte = (unsigned char)bytes[index];

        switch (byte) {
            case '\n':
                fputs("\\n", stdout);
                break;
            case '\t':
                fputs("\\t", stdout);
                break;
            case '\r':
                fputs("\\r", stdout);
                break;
            case '\\':
                fputs("\\\\", stdout);
                break;
            case '"':
                fputs("\\\"", stdout);
                break;
            default:
                if (byte < 0x20 || byte > 0x7e) {
                    printf("\\x%02x", byte);
                } else {
                    putchar(byte);
                }
                break;
        }
    }

    putchar('"');
}

int main(int argc, char **argv) {
    if (!rv_host_initialize_streams()) {
        return EXIT_FAILURE;
    }
    bool check_only = argc == 3 && strcmp(argv[1], "--check") == 0;
    if (argc != 2 && !check_only) {
        fputs("usage: lexer-probe [--check] source\n", stderr);
        return EXIT_FAILURE;
    }

    const char *path = check_only ? argv[2] : argv[1];
    RvSourceError source_error;
    RvSource *source = rv_source_load(rv_allocator_system(), path, &source_error);
    if (source == NULL) {
        rv_diagnostic_load_error(stderr, path, source_error);
        return EXIT_FAILURE;
    }

    RvTokenStream stream;
    RvLexError error;
    if (!rv_lex_source(source, rv_allocator_system(), &stream, &error)) {
        rv_diagnostic_write(stderr, error.span, error.message);
        rv_token_stream_destroy(&stream);
        rv_source_destroy(source);
        return EXIT_FAILURE;
    }

    if (!check_only) {
        for (size_t index = 0; index < rv_token_stream_count(&stream); ++index) {
            const RvToken *token = rv_token_stream_at(&stream, index);
            RvSourcePosition position;
            rv_source_position(source, token->span.start, &position);

            printf("%zu:%zu %s ", position.line, position.column, rv_token_kind_name(token->kind));
            print_bytes(token->spelling, token->length);
            if (token->kind == RV_TOKEN_TEXT_LITERAL) {
                fputs(" -> ", stdout);
                print_bytes(token->text, token->text_length);
            }
            putchar('\n');
        }
    }

    rv_token_stream_destroy(&stream);
    rv_source_destroy(source);
    return ferror(stdout) ? EXIT_FAILURE : EXIT_SUCCESS;
}
