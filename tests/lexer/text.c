#include "test.h"

#include <string.h>

static bool decoded_text_and_raw_spelling(void) {
    const char input[] =
        "\"\" \"hello\\n\\t\\r\\\\\\\"\" \"// text\" "
        "\"\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80\"";
    const char *decoded[] = {"", "hello\n\t\r\\\"", "// text", "\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80"};
    const char *spellings[] = {
        "\"\"", "\"hello\\n\\t\\r\\\\\\\"\"", "\"// text\"", "\"\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80\""
    };

    RvSource *source = rv_source_from_bytes(
        rv_allocator_system(), "text.ruv", input, sizeof(input) - 1, NULL
    );
    CHECK(source != NULL);

    RvTokenStream stream;
    CHECK(rv_lex_source(source, rv_allocator_system(), &stream, NULL));
    CHECK(rv_token_stream_count(&stream) == 5);

    for (size_t index = 0; index < 4; ++index) {
        const RvToken *token = rv_token_stream_at(&stream, index);
        CHECK(token->kind == RV_TOKEN_TEXT_LITERAL);
        CHECK(token->length == strlen(spellings[index]));
        CHECK(strcmp(token->spelling, spellings[index]) == 0);
        CHECK(token->text_length == strlen(decoded[index]));
        CHECK(memcmp(token->text, decoded[index], token->text_length + 1) == 0);
    }

    rv_token_stream_destroy(&stream);
    rv_source_destroy(source);
    return true;
}

static bool utf8_scalar_boundaries(void) {
    const char input[] =
        "\"\xc2\x80\xdf\xbf\xe0\xa0\x80\xed\x9f\xbf\xee\x80\x80\xef\xbf\xbf"
        "\xf0\x90\x80\x80\xf4\x8f\xbf\xbf\"";

    RvSource *source = rv_source_from_bytes(
        rv_allocator_system(), "unicode.ruv", input, sizeof(input) - 1, NULL
    );
    CHECK(source != NULL);

    RvTokenStream stream;
    CHECK(rv_lex_source(source, rv_allocator_system(), &stream, NULL));

    const RvToken *token = rv_token_stream_at(&stream, 0);
    CHECK(token->kind == RV_TOKEN_TEXT_LITERAL);
    CHECK(token->text_length == sizeof(input) - 3);
    CHECK(memcmp(token->text, input + 1, token->text_length) == 0);
    rv_token_stream_destroy(&stream);
    rv_source_destroy(source);
    return true;
}

static bool invalid_utf8_sequences(void) {
    const unsigned char sequences[][4] = {
        {0x80}, {0xc0, 0x80}, {0xc1, 0xbf}, {0xc2}, {0xe0, 0x80, 0x80},
        {0xed, 0xa0, 0x80}, {0xf0, 0x80, 0x80, 0x80}, {0xf4, 0x90, 0x80, 0x80},
        {0xf5, 0x80, 0x80, 0x80}, {0xfe}, {0xff}, {0xe2, 'x', 0xac},
        {0xe2, 0x82}, {0xf0, 0x90, 0x80}
    };
    const size_t lengths[] = {1, 2, 2, 1, 3, 3, 4, 4, 4, 1, 1, 3, 2, 3};

    for (size_t index = 0; index < sizeof(lengths) / sizeof(lengths[0]); ++index) {
        CHECK(test_lex_error(sequences[index], lengths[index], RV_LEX_INVALID_UTF8, 0, 1));

        unsigned char quoted[8] = {'"', 0};
        memcpy(quoted + 1, sequences[index], lengths[index]);
        quoted[lengths[index] + 1] = '"';
        CHECK(test_lex_error(quoted, lengths[index] + 2, RV_LEX_INVALID_UTF8, 1, 2));
    }

    const unsigned char comment[] = {'/', '/', ' ', 0xff};
    CHECK(test_lex_error(comment, sizeof(comment), RV_LEX_INVALID_UTF8, 3, 4));
    return true;
}

static bool invalid_escapes_and_unterminated_text(void) {
    const char *escapes[] = {"\"\\q\"", "\"\\0\"", "\"\\u1234\"", "\"\\x41\"", "\"\\'\""};
    for (size_t index = 0; index < sizeof(escapes) / sizeof(escapes[0]); ++index) {
        CHECK(test_lex_error(escapes[index], strlen(escapes[index]), RV_LEX_INVALID_ESCAPE, 1, 3));
    }

    CHECK(test_lex_error("\"", 1, RV_LEX_UNTERMINATED_TEXT, 0, 1));
    CHECK(test_lex_error("\"abc", 4, RV_LEX_UNTERMINATED_TEXT, 0, 4));
    CHECK(test_lex_error("\"abc\\", 5, RV_LEX_UNTERMINATED_TEXT, 0, 5));
    CHECK(test_lex_error("\"a\\\"", 4, RV_LEX_UNTERMINATED_TEXT, 0, 4));
    CHECK(test_lex_error("\"a\nb\"", 5, RV_LEX_NEWLINE_IN_TEXT, 2, 3));
    CHECK(test_lex_error("\"a\rb\"", 5, RV_LEX_NEWLINE_IN_TEXT, 2, 3));
    CHECK(test_lex_error("\"a\r\nb\"", 6, RV_LEX_NEWLINE_IN_TEXT, 2, 3));
    CHECK(test_lex_error("\"a\\\n", 4, RV_LEX_NEWLINE_IN_TEXT, 3, 4));
    return true;
}

static bool nul_bytes_are_rejected_everywhere(void) {
    const unsigned char plain[] = {'k', 0, 'x'};
    const unsigned char quoted[] = {'"', 'a', 0, '"'};
    const unsigned char comment[] = {'/', '/', 0};

    CHECK(test_lex_error(plain, sizeof(plain), RV_LEX_NUL_BYTE, 1, 2));
    CHECK(test_lex_error(quoted, sizeof(quoted), RV_LEX_NUL_BYTE, 2, 3));
    CHECK(test_lex_error(comment, sizeof(comment), RV_LEX_NUL_BYTE, 2, 3));
    return true;
}

bool test_text(void) {
    return decoded_text_and_raw_spelling()
        && utf8_scalar_boundaries()
        && invalid_utf8_sequences()
        && invalid_escapes_and_unterminated_text()
        && nul_bytes_are_rejected_everywhere();
}
