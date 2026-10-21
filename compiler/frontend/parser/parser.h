#ifndef RUVA_PARSER_H
#define RUVA_PARSER_H

#include "compiler/frontend/ast/ast.h"
#include "compiler/frontend/lexer/lexer.h"

typedef enum {
    RV_PARSE_OK,
    RV_PARSE_INVALID_ARGUMENT,
    RV_PARSE_SYNTAX,
    RV_PARSE_ALLOCATION,
    RV_PARSE_LIMIT
} RvParseErrorKind;

typedef struct {
    RvParseErrorKind kind;
    RvSpan span;
    const char *message;
} RvParseError;

/* Output must be new or destroyed. Failure leaves an empty, destroyable module. */
bool rv_parse_module(const RvTokenStream *tokens, bool entry, RvAllocator allocator,
                      RvAstModule *output, RvParseError *error);

#endif
