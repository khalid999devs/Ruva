#include "token.h"

#include <string.h>

static const char *const token_names[RV_TOKEN_COUNT] = {
    [RV_TOKEN_INVALID] = "invalid",
    [RV_TOKEN_EOF] = "end of file",
    [RV_TOKEN_IDENTIFIER] = "identifier",
    [RV_TOKEN_INTEGER] = "integer",
    [RV_TOKEN_REAL_LITERAL] = "real literal",
    [RV_TOKEN_TEXT_LITERAL] = "text literal",

    [RV_TOKEN_LAUNCH] = "launch",
    [RV_TOKEN_KEEP] = "keep",
    [RV_TOKEN_VARY] = "vary",
    [RV_TOKEN_SHARE] = "share",
    [RV_TOKEN_BRING] = "bring",
    [RV_TOKEN_INT] = "int",
    [RV_TOKEN_REAL] = "real",
    [RV_TOKEN_BOOL] = "bool",
    [RV_TOKEN_TEXT] = "text",
    [RV_TOKEN_VOID] = "void",
    [RV_TOKEN_FILE] = "file",
    [RV_TOKEN_TRUE] = "true",
    [RV_TOKEN_FALSE] = "false",
    [RV_TOKEN_CHECK] = "check",
    [RV_TOKEN_OTHERWISE] = "otherwise",
    [RV_TOKEN_CHOOSE] = "choose",
    [RV_TOKEN_WHEN] = "when",
    [RV_TOKEN_WALK] = "walk",
    [RV_TOKEN_FROM] = "from",
    [RV_TOKEN_UNTIL] = "until",
    [RV_TOKEN_IN] = "in",
    [RV_TOKEN_DURING] = "during",
    [RV_TOKEN_CRAFT] = "craft",
    [RV_TOKEN_GIVE] = "give",
    [RV_TOKEN_OBJECT] = "object",

    [RV_TOKEN_EQUAL] = "=",
    [RV_TOKEN_PLUS] = "+",
    [RV_TOKEN_MINUS] = "-",
    [RV_TOKEN_STAR] = "*",
    [RV_TOKEN_SLASH] = "/",
    [RV_TOKEN_PERCENT] = "%",
    [RV_TOKEN_BANG] = "!",
    [RV_TOKEN_LESS] = "<",
    [RV_TOKEN_GREATER] = ">",
    [RV_TOKEN_EQUAL_EQUAL] = "==",
    [RV_TOKEN_BANG_EQUAL] = "!=",
    [RV_TOKEN_LESS_EQUAL] = "<=",
    [RV_TOKEN_GREATER_EQUAL] = ">=",
    [RV_TOKEN_AND_AND] = "&&",
    [RV_TOKEN_OR_OR] = "||",
    [RV_TOKEN_ARROW] = "->",
    [RV_TOKEN_FAT_ARROW] = "=>",
    [RV_TOKEN_LEFT_PAREN] = "(",
    [RV_TOKEN_RIGHT_PAREN] = ")",
    [RV_TOKEN_LEFT_BRACE] = "{",
    [RV_TOKEN_RIGHT_BRACE] = "}",
    [RV_TOKEN_LEFT_BRACKET] = "[",
    [RV_TOKEN_RIGHT_BRACKET] = "]",
    [RV_TOKEN_COMMA] = ",",
    [RV_TOKEN_COLON] = ":",
    [RV_TOKEN_SEMICOLON] = ";",
    [RV_TOKEN_DOT] = "."
};

const char *rv_token_kind_name(RvTokenKind kind) {
    if (kind < 0 || kind >= RV_TOKEN_COUNT) {
        return "invalid";
    }

    return token_names[kind];
}

RvTokenKind rv_token_keyword(const char *spelling, size_t length) {
    for (RvTokenKind kind = RV_TOKEN_LAUNCH; kind <= RV_TOKEN_OBJECT; ++kind) {
        const char *keyword = token_names[kind];
        if (strlen(keyword) == length && memcmp(keyword, spelling, length) == 0) {
            return kind;
        }
    }

    return RV_TOKEN_IDENTIFIER;
}
