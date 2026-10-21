#include "tests/parser/test.h"

#include <string.h>

static bool allocation_failure_cleanup(void) {
    char input[12000];
    const char prefix[] =
        "bring { Other } from \"other.ruv\";"
        "share object Pair { title: text; count: int; }"
        "craft f(value: Pair) -> Pair { give value; }"
        "launch() {";
    const char statement[] = "keep value = [ask(int), 08, -2147483648]; check ready {} {} ";
    size_t length = sizeof(prefix) - 1;
    memcpy(input, prefix, length);

    for (size_t index = 0; index < 100; ++index) {
        memcpy(input + length, statement, sizeof(statement) - 1);
        length += sizeof(statement) - 1;
    }
    input[length++] = '}';
    input[length] = '\0';

    RvSource *source = rv_source_from_bytes(
        rv_allocator_system(), "allocation.ruv", input, length, NULL
    );
    CHECK(source != NULL);
    RvTokenStream tokens;
    CHECK(rv_lex_source(source, rv_allocator_system(), &tokens, NULL));

    TestAllocator baseline = {0};
    RvAstModule module;
    RvParseError error;
    CHECK(rv_parse_module(&tokens, true, test_allocator(&baseline), &module, &error));
    CHECK(error.kind == RV_PARSE_OK);
    size_t calls = baseline.calls;
    CHECK(calls > 10);
    rv_ast_module_destroy(&module);
    CHECK(baseline.live == 0);

    for (size_t failure = 1; failure <= calls; ++failure) {
        TestAllocator state = {.fail_at = failure};
        CHECK(!rv_parse_module(&tokens, true, test_allocator(&state), &module, &error));
        CHECK(error.kind == RV_PARSE_ALLOCATION && error.message != NULL);
        CHECK(state.live == 0 && module.source == NULL && module.launch == NULL);
        CHECK(memcmp(rv_source_bytes(source), input, length) == 0);
        rv_ast_module_destroy(&module);
    }

    CHECK(rv_parse_module(&tokens, true, rv_allocator_system(), &module, NULL));
    rv_ast_module_destroy(&module);
    rv_ast_module_destroy(&module);
    rv_token_stream_destroy(&tokens);
    rv_source_destroy(source);
    return true;
}

static bool invalid_token_streams(void) {
    RvAstModule module;
    RvParseError error;
    CHECK(!rv_parse_module(NULL, true, rv_allocator_system(), &module, &error));
    CHECK(error.kind == RV_PARSE_INVALID_ARGUMENT && module.source == NULL);
    rv_ast_module_destroy(&module);
    CHECK(!rv_parse_module(NULL, true, rv_allocator_system(), NULL, &error));
    CHECK(error.kind == RV_PARSE_INVALID_ARGUMENT);
    CHECK(!rv_parse_module(NULL, true, rv_allocator_system(), NULL, NULL));

    RvSource *source = rv_source_from_bytes(rv_allocator_system(), "empty.ruv", NULL, 0, NULL);
    CHECK(source != NULL);
    RvTokenStream tokens;
    CHECK(rv_lex_source(source, rv_allocator_system(), &tokens, NULL));

    RvToken *end = rv_vector_at(&tokens.tokens, 0);
    end->kind = RV_TOKEN_IDENTIFIER;
    CHECK(!rv_parse_module(&tokens, false, rv_allocator_system(), &module, &error));
    CHECK(error.kind == RV_PARSE_INVALID_ARGUMENT);
    rv_ast_module_destroy(&module);
    end->kind = RV_TOKEN_EOF;
    CHECK(rv_parse_module(&tokens, false, rv_allocator_system(), &module, NULL));
    CHECK(!rv_ast_dump(NULL, &module));
    rv_ast_module_destroy(&module);
    rv_token_stream_destroy(&tokens);
    rv_source_destroy(source);
    return true;
}

static bool nesting_limits_are_controlled(void) {
    char input[2000];
    const char prefix[] = "launch() { keep value = ";
    size_t length = sizeof(prefix) - 1;
    memcpy(input, prefix, length);
    memset(input + length, '(', 256);
    length += 256;
    input[length++] = '1';
    memset(input + length, ')', 256);
    length += 256;
    memcpy(input + length, "; }", 4);
    CHECK(test_parse_error(input, true, RV_PARSE_LIMIT, "nesting limit"));

    length = sizeof(prefix) - 1;
    memset(input + length, '!', 256);
    length += 256;
    memcpy(input + length, "true; }", 8);
    CHECK(test_parse_error(input, true, RV_PARSE_LIMIT, "nesting limit"));

    length = sizeof(prefix) - 1;
    for (size_t index = 0; index < 256; ++index) {
        input[length++] = '1';
        input[length++] = '+';
    }
    memcpy(input + length, "1; }", 5);
    CHECK(test_parse_error(input, true, RV_PARSE_LIMIT, "nesting limit"));

    memcpy(input, "launch() {", 10);
    length = 10;
    memset(input + length, '{', 256);
    length += 256;
    memset(input + length, '}', 257);
    length += 257;
    input[length] = '\0';
    CHECK(test_parse_error(input, true, RV_PARSE_LIMIT, "nesting limit"));
    return true;
}

static bool large_flat_blocks_do_not_recurse_per_statement(void) {
    char input[40000];
    const char statement[] = "show(1);";
    size_t length = 10;
    memcpy(input, "launch() {", length);
    for (size_t index = 0; index < 4000; ++index) {
        memcpy(input + length, statement, sizeof(statement) - 1);
        length += sizeof(statement) - 1;
    }
    input[length++] = '}';
    input[length] = '\0';

    TestProgram program;
    CHECK(test_parse_program(input, true, &program));
    RvAstSequence *sequence = program.module.launch->as.function.body->as.statements;
    size_t count = 0;
    while (sequence != NULL) {
        CHECK(sequence->alternative == NULL && sequence->statement->kind == RV_AST_EXPRESSION);
        sequence = sequence->next;
        ++count;
    }
    CHECK(count == 4000);
    test_program_destroy(&program);
    return true;
}

static bool mutated_programs_keep_error_cleanup_consistent(void) {
    const char *tokens[] = {
        "launch", "(", ")", "{", "keep", "value", "=", "1", ";",
        "check", "ready", "{", "}", "{", "show", "(", "value", ")", ";", "}",
        "give", ";", "}"
    };
    const char *replacements[] = {
        "keep", "vary", "craft", "choose", "when", "walk", "until", "give",
        "value", "ask", "int", "1", "\"text\"", "+", "=", ";", "(", ")", "{", "}", "[", "]"
    };
    size_t count = sizeof(tokens) / sizeof(tokens[0]);
    size_t replacement_count = sizeof(replacements) / sizeof(replacements[0]);
    uint32_t random = 1701;

    for (size_t iteration = 0; iteration < 512; ++iteration) {
        random = random * 1664525u + 1013904223u;
        size_t changed = random % count;
        const char *replacement = replacements[(random / count) % replacement_count];
        unsigned int operation = (random / 4096) % 3;
        char input[512];
        size_t length = 0;

        for (size_t index = 0; index < count; ++index) {
            if (index == changed && operation == 0) {
                continue;
            }

            const char *word = index == changed && operation == 1 ? replacement : tokens[index];
            size_t copies = index == changed && operation == 2 ? 2 : 1;
            for (size_t copy = 0; copy < copies; ++copy) {
                size_t size = strlen(word);
                memcpy(input + length, word, size);
                length += size;
                input[length++] = ' ';
            }
        }
        input[length] = '\0';

        RvSource *source = rv_source_from_bytes(
            rv_allocator_system(), "mutation.ruv", input, length, NULL
        );
        CHECK(source != NULL);
        RvTokenStream stream;
        CHECK(rv_lex_source(source, rv_allocator_system(), &stream, NULL));
        TestAllocator state = {0};
        RvAstModule module;
        RvParseError error;

        bool success = rv_parse_module(&stream, true, test_allocator(&state), &module, &error);
        if (success) {
            CHECK(error.kind == RV_PARSE_OK && module.launch != NULL);
        } else {
            CHECK(error.kind == RV_PARSE_SYNTAX || error.kind == RV_PARSE_LIMIT);
            CHECK(error.message != NULL && module.source == NULL);
        }

        rv_ast_module_destroy(&module);
        CHECK(state.live == 0);
        rv_token_stream_destroy(&stream);
        rv_source_destroy(source);
    }

    return true;
}

bool test_parser_failures(void) {
    return allocation_failure_cleanup()
        && invalid_token_streams()
        && nesting_limits_are_controlled()
        && large_flat_blocks_do_not_recurse_per_statement()
        && mutated_programs_keep_error_cleanup_consistent();
}
