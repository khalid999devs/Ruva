#ifndef RUVA_LEXER_H
#define RUVA_LEXER_H

#include "token.h"
#include "compiler/common/memory/arena.h"
#include "compiler/common/memory/vector.h"

typedef enum {
    RV_LEX_OK,
    RV_LEX_INVALID_ARGUMENT,
    RV_LEX_ALLOCATION,
    RV_LEX_SOURCE_TOO_LARGE,
    RV_LEX_INVALID_UTF8,
    RV_LEX_NUL_BYTE,
    RV_LEX_UNKNOWN_CHARACTER,
    RV_LEX_INVALID_ESCAPE,
    RV_LEX_UNTERMINATED_TEXT,
    RV_LEX_NEWLINE_IN_TEXT,
    RV_LEX_SCANNER_FAILURE
} RvLexErrorKind;

typedef struct {
    RvLexErrorKind kind;
    RvSpan span;
    const char *message;
} RvLexError;

/* Tokens own their strings through this stream; spans borrow the source. */
typedef struct {
    RvArena storage;
    RvVector tokens;
} RvTokenStream;

/* Output must be new or destroyed. Failure leaves an empty, destroyable stream. */
bool rv_lex_source(const RvSource *source, RvAllocator allocator,
                   RvTokenStream *output, RvLexError *error);
void rv_token_stream_destroy(RvTokenStream *stream);
size_t rv_token_stream_count(const RvTokenStream *stream);
const RvToken *rv_token_stream_at(const RvTokenStream *stream, size_t index);

#endif
