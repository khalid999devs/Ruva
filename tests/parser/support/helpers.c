#include "tests/parser/test.h"
#include "compiler/common/diagnostics/diagnostic.h"

#include <string.h>

bool test_parse_program(const char *input, bool entry, TestProgram *program) {
    program->source = rv_source_from_bytes(
        rv_allocator_system(), "test.ruv", input, strlen(input), NULL
    );
    CHECK(program->source != NULL);

    RvTokenStream tokens;
    RvLexError lex_error;
    CHECK(rv_lex_source(program->source, rv_allocator_system(), &tokens, &lex_error));

    RvParseError error;
    bool success = rv_parse_module(&tokens, entry, rv_allocator_system(), &program->module, &error);
    rv_token_stream_destroy(&tokens);
    if (!success) {
        rv_diagnostic_write(stderr, error.span, error.message);
    }

    CHECK(success && error.kind == RV_PARSE_OK);
    return true;
}

void test_program_destroy(TestProgram *program) {
    rv_ast_module_destroy(&program->module);
    rv_source_destroy(program->source);
    program->source = NULL;
}

bool test_parse_error(const char *input, bool entry, RvParseErrorKind kind, const char *message) {
    RvSource *source = rv_source_from_bytes(
        rv_allocator_system(), "invalid.ruv", input, strlen(input), NULL
    );
    CHECK(source != NULL);

    RvTokenStream tokens;
    CHECK(rv_lex_source(source, rv_allocator_system(), &tokens, NULL));
    TestAllocator state = {0};
    RvAstModule module;
    RvParseError error;

    CHECK(!rv_parse_module(&tokens, entry, test_allocator(&state), &module, &error));
    CHECK(error.kind == kind && error.message != NULL);
    if (message != NULL) {
        CHECK(strstr(error.message, message) != NULL);
    }
    CHECK(error.span.source == source && error.span.start <= error.span.end);
    CHECK(error.span.end <= rv_source_length(source));
    CHECK(module.source == NULL && module.declarations == NULL && module.launch == NULL);
    CHECK(state.live == 0);

    rv_ast_module_destroy(&module);
    rv_token_stream_destroy(&tokens);
    rv_source_destroy(source);
    return true;
}

RvAstExpr *test_initializer(const TestProgram *program, size_t index) {
    RvAstSequence *sequence = program->module.launch->as.function.body->as.statements;
    while (sequence != NULL && index != 0) {
        sequence = sequence->next;
        --index;
    }

    if (sequence == NULL || sequence->statement->kind != RV_AST_DECLARATION) {
        return NULL;
    }
    return sequence->statement->as.declaration.initializer;
}
