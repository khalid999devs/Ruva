#include "lexer.h"
#include "scanner.h"

#include <limits.h>
#include <string.h>

int rv_scan_error(RvScanState *state, RvLexErrorKind kind, size_t start,
                  size_t end, const char *message) {
    state->error = (RvLexError){
        .kind = kind,
        .span = {state->source, start, end},
        .message = message
    };

    return RV_TOKEN_INVALID;
}

static bool validate_source(RvScanState *state) {
    const unsigned char *bytes = rv_source_bytes(state->source);
    size_t length = rv_source_length(state->source);
    size_t offset = 0;

    while (offset < length) {
        unsigned char first = bytes[offset];
        if (first == 0) {
            rv_scan_error(state, RV_LEX_NUL_BYTE, offset, offset + 1,
                          "NUL byte is not allowed in source");
            return false;
        }

        if (first < 0x80) {
            ++offset;
            continue;
        }

        size_t width = 0;
        unsigned char minimum = 0x80;
        unsigned char maximum = 0xbf;

        if (first >= 0xc2 && first <= 0xdf) {
            width = 2;
        } else if (first >= 0xe0 && first <= 0xef) {
            width = 3;
            if (first == 0xe0) {
                minimum = 0xa0;
            } else if (first == 0xed) {
                maximum = 0x9f;
            }
        } else if (first >= 0xf0 && first <= 0xf4) {
            width = 4;
            if (first == 0xf0) {
                minimum = 0x90;
            } else if (first == 0xf4) {
                maximum = 0x8f;
            }
        }

        bool valid = width != 0 && width <= length - offset;
        if (valid) {
            unsigned char second = bytes[offset + 1];
            valid = second >= minimum && second <= maximum;
        }

        for (size_t index = 2; valid && index < width; ++index) {
            unsigned char next = bytes[offset + index];
            valid = next >= 0x80 && next <= 0xbf;
        }

        if (!valid) {
            rv_scan_error(state, RV_LEX_INVALID_UTF8, offset, offset + 1,
                          "invalid UTF-8 in source");
            return false;
        }

        offset += width;
    }

    return true;
}

static bool decode_text(RvScanState *state, RvToken *token) {
    const char *spelling = token->spelling;
    char *decoded = rv_arena_allocate(&state->output->storage, token->length - 1);
    if (decoded == NULL) {
        rv_scan_error(state, RV_LEX_ALLOCATION, state->start, state->offset,
                      "out of memory while tokenizing source");
        return false;
    }

    size_t written = 0;
    for (size_t index = 1; index < token->length - 1; ++index) {
        char byte = spelling[index];
        if (byte == '\\') {
            size_t escape_start = state->start + index;
            ++index;

            switch (spelling[index]) {
                case 'n':
                    byte = '\n';
                    break;
                case 't':
                    byte = '\t';
                    break;
                case 'r':
                    byte = '\r';
                    break;
                case '\\':
                    byte = '\\';
                    break;
                case '"':
                    byte = '"';
                    break;
                default:
                    rv_scan_error(state, RV_LEX_INVALID_ESCAPE, escape_start,
                                  escape_start + 2, "invalid text escape");
                    return false;
            }
        }

        decoded[written++] = byte;
    }

    decoded[written] = '\0';
    token->text = decoded;
    token->text_length = written;
    return true;
}

int rv_scan_emit(RvScanState *state, RvTokenKind kind, const char *bytes, size_t length) {
    char *spelling = rv_arena_copy(&state->output->storage, bytes, length);
    if (spelling == NULL) {
        return rv_scan_error(state, RV_LEX_ALLOCATION, state->start, state->offset,
                             "out of memory while tokenizing source");
    }

    RvToken token = {
        .kind = kind,
        .span = {state->source, state->start, state->offset},
        .spelling = spelling,
        .length = length
    };

    if (kind == RV_TOKEN_TEXT_LITERAL && !decode_text(state, &token)) {
        return RV_TOKEN_INVALID;
    }

    RvToken *slot = rv_vector_append(&state->output->tokens);
    if (slot == NULL) {
        return rv_scan_error(state, RV_LEX_ALLOCATION, state->start, state->offset,
                             "out of memory while tokenizing source");
    }

    *slot = token;
    return kind;
}

int rv_scan_unterminated_text(RvScanState *state) {
    size_t length = rv_source_length(state->source);
    if (state->offset < length) {
        return rv_scan_error(state, RV_LEX_NEWLINE_IN_TEXT, state->offset,
                             state->offset + 1, "literal newline is not allowed in text");
    }

    return rv_scan_error(state, RV_LEX_UNTERMINATED_TEXT, state->start,
                         state->offset, "unterminated text literal");
}

_Noreturn void rv_scan_fatal(RvScanState *state) {
    if (state->error.kind == RV_LEX_OK) {
        rv_scan_error(state, RV_LEX_SCANNER_FAILURE, state->start, state->offset,
                      "scanner failed while tokenizing source");
    }

    longjmp(state->recovery, 1);
}

bool rv_lex_source(const RvSource *source, RvAllocator allocator,
                   RvTokenStream *output, RvLexError *error) {
    if (error != NULL) {
        *error = (RvLexError){0};
    }

    if (output == NULL) {
        if (error != NULL) {
            error->kind = RV_LEX_INVALID_ARGUMENT;
            error->message = "token output is required";
        }
        return false;
    }

    rv_arena_init(&output->storage, allocator, 4096);
    rv_vector_init(&output->tokens, allocator, sizeof(RvToken));

    RvScanState state = {
        .source = source,
        .output = output
    };
    rv_arena_init(&state.scratch, allocator, 1);

    bool success = false;
    if (source == NULL) {
        rv_scan_error(&state, RV_LEX_INVALID_ARGUMENT, 0, 0, "source is required");
    } else if (rv_source_length(source) > (size_t)INT_MAX - 2) {
        rv_scan_error(&state, RV_LEX_SOURCE_TOO_LARGE, 0, 0,
                      "source exceeds the scanner size limit");
    } else if (validate_source(&state)) {
        success = rv_scan_run(&state);
    }

    rv_arena_destroy(&state.scratch);
    if (!success) {
        rv_token_stream_destroy(output);
    }

    if (error != NULL) {
        *error = state.error;
    }

    return success;
}

void rv_token_stream_destroy(RvTokenStream *stream) {
    rv_vector_destroy(&stream->tokens);
    rv_arena_destroy(&stream->storage);
}

size_t rv_token_stream_count(const RvTokenStream *stream) {
    return stream->tokens.length;
}

const RvToken *rv_token_stream_at(const RvTokenStream *stream, size_t index) {
    return rv_vector_at(&stream->tokens, index);
}
