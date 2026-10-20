#include "test.h"

#include <string.h>

static bool keyword_inventory(void) {
    const char *words[] = {
        "launch", "keep", "vary", "share", "bring", "int", "real", "bool", "text",
        "void", "file", "true", "false", "check", "otherwise", "choose", "when",
        "walk", "from", "until", "in", "during", "craft", "give", "object"
    };
    const RvTokenKind kinds[] = {
        RV_TOKEN_LAUNCH, RV_TOKEN_KEEP, RV_TOKEN_VARY, RV_TOKEN_SHARE, RV_TOKEN_BRING,
        RV_TOKEN_INT, RV_TOKEN_REAL, RV_TOKEN_BOOL, RV_TOKEN_TEXT, RV_TOKEN_VOID,
        RV_TOKEN_FILE, RV_TOKEN_TRUE, RV_TOKEN_FALSE, RV_TOKEN_CHECK, RV_TOKEN_OTHERWISE,
        RV_TOKEN_CHOOSE, RV_TOKEN_WHEN, RV_TOKEN_WALK, RV_TOKEN_FROM, RV_TOKEN_UNTIL,
        RV_TOKEN_IN, RV_TOKEN_DURING, RV_TOKEN_CRAFT, RV_TOKEN_GIVE, RV_TOKEN_OBJECT
    };

    for (size_t index = 0; index < sizeof(words) / sizeof(words[0]); ++index) {
        size_t length = strlen(words[index]);
        RvSource *source = rv_source_from_bytes(
            rv_allocator_system(), "keywords.ruv", words[index], length, NULL
        );
        CHECK(source != NULL);

        RvTokenStream stream;
        CHECK(rv_lex_source(source, rv_allocator_system(), &stream, NULL));
        CHECK(rv_token_stream_count(&stream) == 2);

        const RvToken *token = rv_token_stream_at(&stream, 0);
        CHECK(token->kind == kinds[index]);
        CHECK(token->length == length && strcmp(token->spelling, words[index]) == 0);
        CHECK(token->span.source == source && token->span.start == 0 && token->span.end == length);
        CHECK(strcmp(rv_token_kind_name(token->kind), words[index]) == 0);
        CHECK(rv_token_stream_at(&stream, 1)->kind == RV_TOKEN_EOF);

        rv_token_stream_destroy(&stream);
        rv_source_destroy(source);
    }

    return true;
}

static bool identifier_inventory(void) {
    const char input[] =
        "ask show toInt toReal toText toBool push pop length "
        "fileWrite fileAppend fileRead fileExists fileOpen fileReadFrom fileWriteTo fileClose "
        "keeper launchpad Launch KEEP _ _name name2 rv_main __helper malloc printf return class";

    RvSource *source = rv_source_from_bytes(
        rv_allocator_system(), "names.ruv", input, sizeof(input) - 1, NULL
    );
    CHECK(source != NULL);

    RvTokenStream stream;
    CHECK(rv_lex_source(source, rv_allocator_system(), &stream, NULL));
    CHECK(rv_token_stream_count(&stream) == 31);

    const char *word = input;
    for (size_t index = 0; index + 1 < rv_token_stream_count(&stream); ++index) {
        const char *end = strchr(word, ' ');
        size_t length = end == NULL ? strlen(word) : (size_t)(end - word);
        const RvToken *token = rv_token_stream_at(&stream, index);

        CHECK(token->kind == RV_TOKEN_IDENTIFIER);
        CHECK(token->length == length && memcmp(token->spelling, word, length) == 0);
        word += length;
        if (*word == ' ') {
            ++word;
        }
    }

    CHECK(*word == '\0');
    CHECK(rv_token_stream_at(&stream, 30)->kind == RV_TOKEN_EOF);
    rv_token_stream_destroy(&stream);
    rv_source_destroy(source);
    return true;
}

static bool operators_and_longest_matches(void) {
    const char input[] = "= + - * / % ! < > == != <= >= && || -> => ( ) { } [ ] , : ; . === !== ->=>";
    const RvTokenKind kinds[] = {
        RV_TOKEN_EQUAL, RV_TOKEN_PLUS, RV_TOKEN_MINUS, RV_TOKEN_STAR, RV_TOKEN_SLASH,
        RV_TOKEN_PERCENT, RV_TOKEN_BANG, RV_TOKEN_LESS, RV_TOKEN_GREATER,
        RV_TOKEN_EQUAL_EQUAL, RV_TOKEN_BANG_EQUAL, RV_TOKEN_LESS_EQUAL, RV_TOKEN_GREATER_EQUAL,
        RV_TOKEN_AND_AND, RV_TOKEN_OR_OR, RV_TOKEN_ARROW, RV_TOKEN_FAT_ARROW,
        RV_TOKEN_LEFT_PAREN, RV_TOKEN_RIGHT_PAREN, RV_TOKEN_LEFT_BRACE, RV_TOKEN_RIGHT_BRACE,
        RV_TOKEN_LEFT_BRACKET, RV_TOKEN_RIGHT_BRACKET, RV_TOKEN_COMMA, RV_TOKEN_COLON,
        RV_TOKEN_SEMICOLON, RV_TOKEN_DOT, RV_TOKEN_EQUAL_EQUAL, RV_TOKEN_EQUAL,
        RV_TOKEN_BANG_EQUAL, RV_TOKEN_EQUAL, RV_TOKEN_ARROW, RV_TOKEN_FAT_ARROW,
        RV_TOKEN_EOF
    };

    RvSource *source = rv_source_from_bytes(
        rv_allocator_system(), "operators.ruv", input, sizeof(input) - 1, NULL
    );
    CHECK(source != NULL);

    RvTokenStream stream;
    CHECK(rv_lex_source(source, rv_allocator_system(), &stream, NULL));
    CHECK(rv_token_stream_count(&stream) == sizeof(kinds) / sizeof(kinds[0]));

    for (size_t index = 0; index < rv_token_stream_count(&stream); ++index) {
        const RvToken *token = rv_token_stream_at(&stream, index);
        CHECK(token->kind == kinds[index]);
        if (token->kind != RV_TOKEN_EOF) {
            CHECK(strcmp(token->spelling, rv_token_kind_name(token->kind)) == 0);
        }
    }

    rv_token_stream_destroy(&stream);
    rv_source_destroy(source);
    return true;
}

static bool numbers_preserve_decimal_spelling(void) {
    const char input[] =
        "010 08 001.050 -2147483648 2147483648 999999999999999999999999999999 "
        "1e3 .5 5. 0x10 +25";
    const char *spellings[] = {
        "010", "08", "001.050", "-", "2147483648", "2147483648",
        "999999999999999999999999999999", "1", "e3", ".", "5", "5", ".", "0", "x10", "+", "25", ""
    };
    const RvTokenKind kinds[] = {
        RV_TOKEN_INTEGER, RV_TOKEN_INTEGER, RV_TOKEN_REAL_LITERAL, RV_TOKEN_MINUS,
        RV_TOKEN_INTEGER, RV_TOKEN_INTEGER, RV_TOKEN_INTEGER, RV_TOKEN_INTEGER,
        RV_TOKEN_IDENTIFIER, RV_TOKEN_DOT, RV_TOKEN_INTEGER, RV_TOKEN_INTEGER,
        RV_TOKEN_DOT, RV_TOKEN_INTEGER, RV_TOKEN_IDENTIFIER, RV_TOKEN_PLUS,
        RV_TOKEN_INTEGER, RV_TOKEN_EOF
    };

    RvSource *source = rv_source_from_bytes(
        rv_allocator_system(), "numbers.ruv", input, sizeof(input) - 1, NULL
    );
    CHECK(source != NULL);

    RvTokenStream stream;
    CHECK(rv_lex_source(source, rv_allocator_system(), &stream, NULL));
    CHECK(rv_token_stream_count(&stream) == sizeof(kinds) / sizeof(kinds[0]));

    for (size_t index = 0; index < rv_token_stream_count(&stream); ++index) {
        const RvToken *token = rv_token_stream_at(&stream, index);
        CHECK(token->kind == kinds[index]);
        CHECK(strcmp(token->spelling, spellings[index]) == 0);
        CHECK(token->text == NULL && token->text_length == 0);
    }

    CHECK(memcmp(rv_source_bytes(source), input, sizeof(input)) == 0);
    rv_token_stream_destroy(&stream);
    rv_source_destroy(source);
    return true;
}

static bool positions_after_comments_and_line_endings(void) {
    const char input[] = "// hi\r\n\tkeep a = \"\xc3\xa9\";\rshow(a)\n";
    const size_t starts[] = {8, 13, 15, 17, 21, 23, 27, 28, 29, 31};
    const size_t ends[] = {12, 14, 16, 21, 22, 27, 28, 29, 30, 31};
    const RvSourcePosition positions[] = {
        {2, 2}, {2, 7}, {2, 9}, {2, 11}, {2, 15},
        {3, 1}, {3, 5}, {3, 6}, {3, 7}, {4, 1}
    };

    RvSource *source = rv_source_from_bytes(
        rv_allocator_system(), "positions.ruv", input, sizeof(input) - 1, NULL
    );
    CHECK(source != NULL);

    RvTokenStream stream;
    CHECK(rv_lex_source(source, rv_allocator_system(), &stream, NULL));
    CHECK(rv_token_stream_count(&stream) == sizeof(starts) / sizeof(starts[0]));

    for (size_t index = 0; index < rv_token_stream_count(&stream); ++index) {
        const RvToken *token = rv_token_stream_at(&stream, index);
        RvSourcePosition position;

        CHECK(token->span.start == starts[index] && token->span.end == ends[index]);
        CHECK(rv_source_position(source, token->span.start, &position));
        CHECK(position.line == positions[index].line && position.column == positions[index].column);
    }

    CHECK(rv_token_stream_at(&stream, 3)->text_length == 2);
    rv_token_stream_destroy(&stream);
    rv_source_destroy(source);
    return true;
}

static bool streams_keep_independent_owned_tokens(void) {
    const char first_input[] = "keep first = \"saved\";";
    RvSource *first = rv_source_from_bytes(
        rv_allocator_system(), "first.ruv", first_input, sizeof(first_input) - 1, NULL
    );
    RvSource *second = rv_source_from_bytes(
        rv_allocator_system(), "second.ruv", "vary second", 11, NULL
    );
    CHECK(first != NULL && second != NULL);

    RvTokenStream first_stream;
    RvTokenStream second_stream;
    CHECK(rv_lex_source(first, rv_allocator_system(), &first_stream, NULL));

    const RvToken *name = rv_token_stream_at(&first_stream, 1);
    const RvToken *text = rv_token_stream_at(&first_stream, 3);
    CHECK(rv_lex_source(second, rv_allocator_system(), &second_stream, NULL));
    rv_token_stream_destroy(&second_stream);
    rv_source_destroy(second);

    CHECK(strcmp(name->spelling, "first") == 0);
    CHECK(strcmp(text->spelling, "\"saved\"") == 0 && strcmp(text->text, "saved") == 0);
    CHECK(name->span.source == first && text->span.source == first);
    CHECK(rv_token_stream_at(&first_stream, rv_token_stream_count(&first_stream)) == NULL);
    CHECK(memcmp(rv_source_bytes(first), first_input, sizeof(first_input)) == 0);

    rv_token_stream_destroy(&first_stream);
    rv_token_stream_destroy(&first_stream);
    CHECK(rv_lex_source(first, rv_allocator_system(), &first_stream, NULL));
    rv_token_stream_destroy(&first_stream);
    rv_source_destroy(first);
    return true;
}

static bool empty_and_comment_only_input(void) {
    const char *inputs[] = {"", " \t\r\n", "// no final newline", "// \"\\q @ \xc3\xa9\r\n"};
    for (size_t index = 0; index < sizeof(inputs) / sizeof(inputs[0]); ++index) {
        size_t length = strlen(inputs[index]);
        RvSource *source = rv_source_from_bytes(
            rv_allocator_system(), "empty.ruv", inputs[index], length, NULL
        );
        CHECK(source != NULL);

        RvTokenStream stream;
        CHECK(rv_lex_source(source, rv_allocator_system(), &stream, NULL));
        CHECK(rv_token_stream_count(&stream) == 1);

        const RvToken *end = rv_token_stream_at(&stream, 0);
        CHECK(end->kind == RV_TOKEN_EOF && end->length == 0 && end->spelling[0] == '\0');
        CHECK(end->span.start == length && end->span.end == length);

        rv_token_stream_destroy(&stream);
        rv_source_destroy(source);
    }

    return true;
}

bool test_tokens(void) {
    return keyword_inventory()
        && identifier_inventory()
        && operators_and_longest_matches()
        && numbers_preserve_decimal_spelling()
        && positions_after_comments_and_line_endings()
        && streams_keep_independent_owned_tokens()
        && empty_and_comment_only_input();
}
