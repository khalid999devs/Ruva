#ifndef RUVA_PARSER_INTERNAL_H
#define RUVA_PARSER_INTERNAL_H

#include "parser.h"

#define RV_PARSE_MAX_DEPTH 128

typedef struct {
    unsigned int status;
    RvAstSequence *sequence;
    RvParseError error;
} RvParseMemo;

typedef struct {
    RvAstModule *module;
    RvToken *tokens;
    size_t count;
    size_t cursor;
    size_t limit;
    size_t depth;
    size_t work;
    size_t work_limit;
    size_t *closing;
    RvParseMemo *memo;
    RvArena scratch;
    RvParseError error;
    bool fatal;
} RvParser;

typedef enum {
    RV_PARSE_LOCAL_TYPE,
    RV_PARSE_PRIMITIVE_TYPE,
    RV_PARSE_PARAMETER_TYPE,
    RV_PARSE_RESULT_TYPE
} RvParseTypeContext;

const RvToken *rv_parse_peek(const RvParser *parser);
const RvToken *rv_parse_take(RvParser *parser);
bool rv_parse_match(RvParser *parser, RvTokenKind kind);
const RvToken *rv_parse_expect(RvParser *parser, RvTokenKind kind, const char *message);
void rv_parse_fail(RvParser *parser, RvParseErrorKind kind, RvSpan span, const char *message);
void rv_parse_keep_error(RvParseError *best, RvParseError candidate);
void *rv_parse_new(RvParser *parser, size_t size);
void *rv_parse_scratch(RvParser *parser, size_t size);
bool rv_parse_enter(RvParser *parser);
RvSpan rv_parse_span(const RvParser *parser, size_t start);
bool rv_parse_type(RvParser *parser, RvParseTypeContext context, RvAstType *type);
RvAstExpr *rv_parse_expression(RvParser *parser);
RvAstExpr *rv_parse_constant(RvParser *parser);
RvAstExpr *rv_parse_expr_new(RvParser *parser, RvAstExprKind kind,
                             RvSpan span, size_t child_depth);
RvAstStmt *rv_parse_block(RvParser *parser);
bool rv_parse_declarations(RvParser *parser);

#endif
