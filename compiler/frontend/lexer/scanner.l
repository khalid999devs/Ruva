%option reentrant noyywrap noinput nounput never-interactive nodefault 8bit
%option noyyalloc noyyrealloc noyyfree
%option prefix="rv_raw_" extra-type="RvScanState *"

%{
#include "compiler/frontend/lexer/scanner.h"

#include <string.h>

#define YY_USER_ACTION \
    yyextra->start = yyextra->offset; \
    yyextra->offset += (size_t)yyleng;

/* Flex's fatal path must return through our cleanup boundary. */
#define YY_FATAL_ERROR(message) do { \
    (void)yy_fatal_error; \
    rv_scan_fatal(yyget_extra(yyscanner)); \
} while (0)
%}

DIGITS      [0-9]+
IDENTIFIER  [A-Za-z_][A-Za-z0-9_]*
TEXT_BODY   ([^\"\\\r\n]|\\[^\r\n])

%%

[ \t\r\n]+ {
}

"//"[^\r\n]* {
}

{IDENTIFIER} {
    RvTokenKind kind = rv_token_keyword(yytext, (size_t)yyleng);
    return rv_scan_emit(yyextra, kind, yytext, (size_t)yyleng);
}

{DIGITS}"."{DIGITS} {
    return rv_scan_emit(yyextra, RV_TOKEN_REAL_LITERAL, yytext, (size_t)yyleng);
}

{DIGITS} {
    return rv_scan_emit(yyextra, RV_TOKEN_INTEGER, yytext, (size_t)yyleng);
}

\"{TEXT_BODY}*\" {
    return rv_scan_emit(yyextra, RV_TOKEN_TEXT_LITERAL, yytext, (size_t)yyleng);
}

\"{TEXT_BODY}*\\? {
    return rv_scan_unterminated_text(yyextra);
}

"==" {
    return rv_scan_emit(yyextra, RV_TOKEN_EQUAL_EQUAL, yytext, (size_t)yyleng);
}

"!=" {
    return rv_scan_emit(yyextra, RV_TOKEN_BANG_EQUAL, yytext, (size_t)yyleng);
}

"<=" {
    return rv_scan_emit(yyextra, RV_TOKEN_LESS_EQUAL, yytext, (size_t)yyleng);
}

">=" {
    return rv_scan_emit(yyextra, RV_TOKEN_GREATER_EQUAL, yytext, (size_t)yyleng);
}

"&&" {
    return rv_scan_emit(yyextra, RV_TOKEN_AND_AND, yytext, (size_t)yyleng);
}

"||" {
    return rv_scan_emit(yyextra, RV_TOKEN_OR_OR, yytext, (size_t)yyleng);
}

"->" {
    return rv_scan_emit(yyextra, RV_TOKEN_ARROW, yytext, (size_t)yyleng);
}

"=>" {
    return rv_scan_emit(yyextra, RV_TOKEN_FAT_ARROW, yytext, (size_t)yyleng);
}

"=" {
    return rv_scan_emit(yyextra, RV_TOKEN_EQUAL, yytext, (size_t)yyleng);
}

"+" {
    return rv_scan_emit(yyextra, RV_TOKEN_PLUS, yytext, (size_t)yyleng);
}

"-" {
    return rv_scan_emit(yyextra, RV_TOKEN_MINUS, yytext, (size_t)yyleng);
}

"*" {
    return rv_scan_emit(yyextra, RV_TOKEN_STAR, yytext, (size_t)yyleng);
}

"/" {
    return rv_scan_emit(yyextra, RV_TOKEN_SLASH, yytext, (size_t)yyleng);
}

"%" {
    return rv_scan_emit(yyextra, RV_TOKEN_PERCENT, yytext, (size_t)yyleng);
}

"!" {
    return rv_scan_emit(yyextra, RV_TOKEN_BANG, yytext, (size_t)yyleng);
}

"<" {
    return rv_scan_emit(yyextra, RV_TOKEN_LESS, yytext, (size_t)yyleng);
}

">" {
    return rv_scan_emit(yyextra, RV_TOKEN_GREATER, yytext, (size_t)yyleng);
}

"(" {
    return rv_scan_emit(yyextra, RV_TOKEN_LEFT_PAREN, yytext, (size_t)yyleng);
}

")" {
    return rv_scan_emit(yyextra, RV_TOKEN_RIGHT_PAREN, yytext, (size_t)yyleng);
}

"{" {
    return rv_scan_emit(yyextra, RV_TOKEN_LEFT_BRACE, yytext, (size_t)yyleng);
}

"}" {
    return rv_scan_emit(yyextra, RV_TOKEN_RIGHT_BRACE, yytext, (size_t)yyleng);
}

"[" {
    return rv_scan_emit(yyextra, RV_TOKEN_LEFT_BRACKET, yytext, (size_t)yyleng);
}

"]" {
    return rv_scan_emit(yyextra, RV_TOKEN_RIGHT_BRACKET, yytext, (size_t)yyleng);
}

"," {
    return rv_scan_emit(yyextra, RV_TOKEN_COMMA, yytext, (size_t)yyleng);
}

":" {
    return rv_scan_emit(yyextra, RV_TOKEN_COLON, yytext, (size_t)yyleng);
}

";" {
    return rv_scan_emit(yyextra, RV_TOKEN_SEMICOLON, yytext, (size_t)yyleng);
}

"." {
    return rv_scan_emit(yyextra, RV_TOKEN_DOT, yytext, (size_t)yyleng);
}

. {
    return rv_scan_error(yyextra, RV_LEX_UNKNOWN_CHARACTER, yyextra->start,
                         yyextra->offset, "unknown source character");
}

<<EOF>> {
    yyextra->start = yyextra->offset;
    return rv_scan_emit(yyextra, RV_TOKEN_EOF, "", 0);
}

%%

typedef union {
    max_align_t alignment;
    size_t size;
} RvScanAllocation;

void *yyalloc(yy_size_t size, yyscan_t scanner) {
    RvScanState *state = yyget_extra(scanner);
    size_t total;
    RvScanAllocation *allocation = NULL;

    if (rv_size_add(sizeof(*allocation), size, &total)) {
        allocation = rv_arena_allocate(&state->scratch, total);
    }

    if (allocation == NULL) {
        rv_scan_error(state, RV_LEX_ALLOCATION, state->start, state->offset,
                      "out of memory while tokenizing source");
        rv_scan_fatal(state);
    }

    allocation->size = size;
    return allocation + 1;
}

void *yyrealloc(void *pointer, yy_size_t size, yyscan_t scanner) {
    void *replacement = yyalloc(size, scanner);
    if (pointer != NULL) {
        RvScanAllocation *allocation = (RvScanAllocation *)pointer - 1;
        size_t copied = allocation->size < size ? allocation->size : size;
        memcpy(replacement, pointer, copied);
    }

    return replacement;
}

void yyfree(void *pointer, yyscan_t scanner) {
    /* The scratch arena releases all scanner allocations together. */
    (void)pointer;
    (void)scanner;
}

bool rv_scan_run(RvScanState *state) {
    if (setjmp(state->recovery) != 0) {
        return false;
    }

    size_t length = rv_source_length(state->source);
    char *buffer = rv_arena_allocate(&state->scratch, length + 2);
    if (buffer == NULL) {
        rv_scan_error(state, RV_LEX_ALLOCATION, 0, 0,
                      "out of memory while tokenizing source");
        return false;
    }

    memcpy(buffer, rv_source_bytes(state->source), length);
    buffer[length] = '\0';
    buffer[length + 1] = '\0';

    yyscan_t scanner = NULL;
    if (yylex_init_extra(state, &scanner) != 0) {
        rv_scan_error(state, RV_LEX_SCANNER_FAILURE, 0, 0, "cannot initialize scanner");
        return false;
    }

    if (yy_scan_buffer(buffer, length + 2, scanner) == NULL) {
        yylex_destroy(scanner);
        rv_scan_error(state, RV_LEX_SCANNER_FAILURE, 0, 0, "cannot initialize source buffer");
        return false;
    }

    int kind;
    do {
        kind = yylex(scanner);
    } while (kind != RV_TOKEN_EOF && kind != RV_TOKEN_INVALID);

    yylex_destroy(scanner);
    return kind == RV_TOKEN_EOF;
}
