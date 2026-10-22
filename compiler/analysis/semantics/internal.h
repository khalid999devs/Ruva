#ifndef RUVA_CHECK_INTERNAL_H
#define RUVA_CHECK_INTERNAL_H

#include "check.h"

typedef struct RvScopeBinding {
    const char *name;
    RvSymbol *symbol;
    struct RvScopeBinding *next;
} RvScopeBinding;

struct RvScope {
    RvScope *parent;
    RvScopeBinding *buckets[32];
};

typedef struct RvActiveWalk {
    RvSymbol *array;
    struct RvActiveWalk *next;
} RvActiveWalk;

typedef struct {
    RvCheckedProgram *program;
    RvCheckError *error;
    RvScope *builtins;
    RvModuleUnit *unit;
    RvScope *scope;
    RvType result;
    RvActiveWalk *walks;
    size_t steps;
    size_t step_limit;
    size_t depth;
} RvChecker;

bool rv_check_fail(RvChecker *checker, RvCheckErrorKind kind, RvSpan span, const char *message);
void *rv_check_new(RvChecker *checker, size_t size, RvSpan span);
bool rv_check_step(RvChecker *checker, RvSpan span);
RvScope *rv_check_scope(RvChecker *checker, RvScope *parent, RvSpan span);
RvSymbol *rv_scope_local(const RvScope *scope, const char *name);
RvSymbol *rv_scope_lookup(const RvScope *scope, const char *name);
bool rv_scope_insert(RvChecker *checker, RvScope *scope, const char *name,
                      RvSymbol *symbol, RvSpan span);
RvSymbol *rv_check_symbol(RvChecker *checker, RvSymbolKind kind, const RvToken *name);
bool rv_check_define(RvChecker *checker, RvSymbol *symbol);
bool rv_check_builtins(RvChecker *checker);
const char *rv_builtin_name(RvBuiltin builtin);
bool rv_check_declarations(RvChecker *checker);
bool rv_check_bodies(RvChecker *checker);
RvType rv_check_type(RvChecker *checker, RvAstType type);
RvType rv_check_expression(RvChecker *checker, RvAstExpr *expression, RvType expected);
RvType rv_check_call(RvChecker *checker, RvAstExpr *expression);
RvType rv_check_record(RvChecker *checker, RvAstExpr *expression);
RvField *rv_check_field(RvSymbol *record, const char *name);
RvAstExpr *rv_check_ungroup(RvAstExpr *expression);
bool rv_check_fresh(RvChecker *checker, RvAstExpr *expression, RvType type);
bool rv_check_mutation(RvChecker *checker, RvSymbol *symbol, RvSpan span, bool resize);
bool rv_check_compatible(RvChecker *checker, const RvAstExpr *expression);
bool rv_check_block(RvChecker *checker, RvAstStmt *block, RvScope *scope, bool *returns);
bool rv_check_integer(RvChecker *checker, RvAstExpr *expression, bool allow_minimum);
bool rv_check_real(RvChecker *checker, RvAstExpr *expression);

#endif
