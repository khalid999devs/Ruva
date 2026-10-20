#include "test.h"

#include <string.h>

bool test_lex_error(const void *bytes, size_t length, RvLexErrorKind kind,
                    size_t start, size_t end) {
    RvSource *source = rv_source_from_bytes(
        rv_allocator_system(), "invalid.ruv", bytes, length, NULL
    );
    CHECK(source != NULL);

    TestAllocator state = {0};
    RvTokenStream stream;
    RvLexError error;
    CHECK(!rv_lex_source(source, test_allocator(&state), &stream, &error));
    CHECK(error.kind == kind && error.message != NULL);
    CHECK(error.span.source == source && error.span.start == start && error.span.end == end);
    CHECK(rv_token_stream_count(&stream) == 0 && state.live == 0);
    CHECK(memcmp(rv_source_bytes(source), bytes, length) == 0);

    rv_token_stream_destroy(&stream);
    rv_source_destroy(source);
    return true;
}

static bool unknown_characters(void) {
    const char *characters[] = {"@", "#", "&", "|", "?", "'", "\\", "\f", "\v", "\xc3\xa9"};
    for (size_t index = 0; index < sizeof(characters) / sizeof(characters[0]); ++index) {
        CHECK(test_lex_error(
            characters[index], strlen(characters[index]), RV_LEX_UNKNOWN_CHARACTER, 0, 1
        ));
    }

    CHECK(test_lex_error("keep name = @;", 14, RV_LEX_UNKNOWN_CHARACTER, 12, 13));
    return true;
}

static bool allocation_failures_release_everything(void) {
    char input[18000];
    const char statement[] = "keep name = \"value\\n\";\n";
    size_t length = 0;

    for (size_t index = 0; index < 200; ++index) {
        memcpy(input + length, statement, sizeof(statement) - 1);
        length += sizeof(statement) - 1;
    }

    input[length++] = '"';
    memset(input + length, 'x', 10000);
    length += 10000;
    input[length++] = '"';

    RvSource *source = rv_source_from_bytes(
        rv_allocator_system(), "allocation.ruv", input, length, NULL
    );
    CHECK(source != NULL);

    TestAllocator baseline = {0};
    RvTokenStream stream;
    RvLexError error;
    CHECK(rv_lex_source(source, test_allocator(&baseline), &stream, &error));
    CHECK(error.kind == RV_LEX_OK && error.message == NULL);
    CHECK(rv_token_stream_count(&stream) == 1002);

    const RvToken *large_text = rv_token_stream_at(&stream, 1000);
    CHECK(large_text->text_length == 10000);
    CHECK(memcmp(large_text->text, input + length - 10001, 10000) == 0);

    size_t calls = baseline.calls;
    CHECK(calls > 10);
    rv_token_stream_destroy(&stream);
    CHECK(baseline.live == 0);

    for (size_t failure = 1; failure <= calls; ++failure) {
        TestAllocator state = {.fail_at = failure};
        CHECK(!rv_lex_source(source, test_allocator(&state), &stream, &error));
        CHECK(error.kind == RV_LEX_ALLOCATION && error.message != NULL);
        CHECK(rv_token_stream_count(&stream) == 0 && state.live == 0);
        CHECK(memcmp(rv_source_bytes(source), input, length) == 0);
        rv_token_stream_destroy(&stream);
    }

    rv_source_destroy(source);
    return true;
}

static bool invalid_arguments_leave_safe_output(void) {
    RvTokenStream stream;
    RvLexError error;

    CHECK(!rv_lex_source(NULL, rv_allocator_system(), &stream, &error));
    CHECK(error.kind == RV_LEX_INVALID_ARGUMENT && error.span.source == NULL);
    CHECK(rv_token_stream_count(&stream) == 0);
    rv_token_stream_destroy(&stream);

    CHECK(!rv_lex_source(NULL, rv_allocator_system(), NULL, &error));
    CHECK(error.kind == RV_LEX_INVALID_ARGUMENT);
    CHECK(!rv_lex_source(NULL, rv_allocator_system(), NULL, NULL));
    return true;
}

bool test_lexer_failure(void) {
    return unknown_characters()
        && allocation_failures_release_everything()
        && invalid_arguments_leave_safe_output();
}
