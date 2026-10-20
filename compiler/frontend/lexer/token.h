#ifndef RUVA_TOKEN_H
#define RUVA_TOKEN_H

#include "compiler/common/source/span.h"

typedef enum {
    RV_TOKEN_INVALID,
    RV_TOKEN_EOF,
    RV_TOKEN_IDENTIFIER,
    RV_TOKEN_INTEGER,
    RV_TOKEN_REAL_LITERAL,
    RV_TOKEN_TEXT_LITERAL,

    RV_TOKEN_LAUNCH,
    RV_TOKEN_KEEP,
    RV_TOKEN_VARY,
    RV_TOKEN_SHARE,
    RV_TOKEN_BRING,
    RV_TOKEN_INT,
    RV_TOKEN_REAL,
    RV_TOKEN_BOOL,
    RV_TOKEN_TEXT,
    RV_TOKEN_VOID,
    RV_TOKEN_FILE,
    RV_TOKEN_TRUE,
    RV_TOKEN_FALSE,
    RV_TOKEN_CHECK,
    RV_TOKEN_OTHERWISE,
    RV_TOKEN_CHOOSE,
    RV_TOKEN_WHEN,
    RV_TOKEN_WALK,
    RV_TOKEN_FROM,
    RV_TOKEN_UNTIL,
    RV_TOKEN_IN,
    RV_TOKEN_DURING,
    RV_TOKEN_CRAFT,
    RV_TOKEN_GIVE,
    RV_TOKEN_OBJECT,

    RV_TOKEN_EQUAL,
    RV_TOKEN_PLUS,
    RV_TOKEN_MINUS,
    RV_TOKEN_STAR,
    RV_TOKEN_SLASH,
    RV_TOKEN_PERCENT,
    RV_TOKEN_BANG,
    RV_TOKEN_LESS,
    RV_TOKEN_GREATER,
    RV_TOKEN_EQUAL_EQUAL,
    RV_TOKEN_BANG_EQUAL,
    RV_TOKEN_LESS_EQUAL,
    RV_TOKEN_GREATER_EQUAL,
    RV_TOKEN_AND_AND,
    RV_TOKEN_OR_OR,
    RV_TOKEN_ARROW,
    RV_TOKEN_FAT_ARROW,
    RV_TOKEN_LEFT_PAREN,
    RV_TOKEN_RIGHT_PAREN,
    RV_TOKEN_LEFT_BRACE,
    RV_TOKEN_RIGHT_BRACE,
    RV_TOKEN_LEFT_BRACKET,
    RV_TOKEN_RIGHT_BRACKET,
    RV_TOKEN_COMMA,
    RV_TOKEN_COLON,
    RV_TOKEN_SEMICOLON,
    RV_TOKEN_DOT,
    RV_TOKEN_COUNT
} RvTokenKind;

typedef struct {
    RvTokenKind kind;
    RvSpan span;
    const char *spelling;
    size_t length;
    const char *text;
    size_t text_length;
} RvToken;

const char *rv_token_kind_name(RvTokenKind kind);
RvTokenKind rv_token_keyword(const char *spelling, size_t length);

#endif
