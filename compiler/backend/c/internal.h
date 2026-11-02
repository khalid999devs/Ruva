#ifndef RUVA_C_INTERNAL_H
#define RUVA_C_INTERNAL_H

#include "emit.h"

typedef struct {
    size_t id;
    RvTypeKind kind;
    RvSymbol *record;
    bool borrowed;
} RvCValue;

typedef struct RvCProjection {
    RvSymbol *symbol;
    RvCValue key;
    RvCValue value;
    struct RvCProjection *parent;
} RvCProjection;

typedef struct RvCLocal {
    RvSymbol *symbol;
    RvCValue temporary;
    struct RvCLocal *next;
} RvCLocal;

typedef struct RvCScope {
    struct RvCScope *parent;
    RvCLocal *locals;
} RvCScope;

typedef struct {
    FILE *output;
    RvArena storage;
    RvEmitError *error;
    RvCScope *scope;
    RvSymbol *function;
    RvCProjection *projection;
    size_t next_temporary;
    size_t indentation;
    size_t generated_bytes;
    RvSpan current_span;
} RvCEmitter;

bool rv_c_fail(RvEmitError *error, RvEmitErrorKind kind, RvSpan span, const char *message);
bool rv_c_write(RvCEmitter *emitter, const char *format, ...);
void rv_c_indent(RvCEmitter *emitter);
void *rv_c_new(RvCEmitter *emitter, size_t size, RvSpan span);
void rv_c_string(RvCEmitter *emitter, const char *bytes, size_t length);
void rv_c_location(RvCEmitter *emitter, RvSpan span);
const char *rv_c_type(RvTypeKind kind);
RvCValue rv_c_expression(RvCEmitter *emitter, const RvAstExpr *expression);
RvCValue rv_c_new_value(RvCEmitter *emitter, RvTypeKind kind);
RvCValue rv_c_new_typed_value(RvCEmitter *emitter, RvType type);
void rv_c_write_type(RvCEmitter *emitter, RvType type);
void rv_c_release_named(RvCEmitter *emitter, RvType type, const char *prefix, size_t id);
void rv_c_retain_record(RvCEmitter *emitter, RvCValue value, RvSpan span);
void rv_c_records(RvCEmitter *emitter, const RvCheckedProgram *program);
RvCValue rv_c_record(RvCEmitter *emitter, const RvAstExpr *expression);
RvCValue rv_c_field(RvCEmitter *emitter, const RvAstExpr *expression);
RvCValue rv_c_array(RvCEmitter *emitter, const RvAstExpr *expression);
RvCValue rv_c_array_call(RvCEmitter *emitter, const RvAstExpr *expression);
RvCValue rv_c_index(RvCEmitter *emitter, const RvAstExpr *expression);
const char *rv_c_element(RvTypeKind kind);
void rv_c_assignment(RvCEmitter *emitter, const RvAstStmt *statement);
void rv_c_each_walk(RvCEmitter *emitter, const RvAstStmt *statement);
void rv_c_own_local(RvCEmitter *emitter, RvSymbol *symbol, RvSpan span);
const char *rv_c_builtin_helper(RvBuiltin builtin, RvTypeKind input);
RvCValue rv_c_ask(RvCEmitter *emitter, const RvAstExpr *expression);
RvCValue rv_c_call(RvCEmitter *emitter, const RvAstExpr *expression);
void rv_c_release(RvCEmitter *emitter, RvCValue value);
void rv_c_discard(RvCEmitter *emitter, RvCValue value);
RvCScope *rv_c_scope_begin(RvCEmitter *emitter, RvSpan span);
void rv_c_scope_end(RvCEmitter *emitter, RvCScope *scope, bool returned);
void rv_c_own_temporary(RvCEmitter *emitter, RvCValue value, RvSpan span);
bool rv_c_block(RvCEmitter *emitter, const RvAstStmt *block);
bool rv_c_control(RvCEmitter *emitter, const RvAstStmt *statement);
void rv_c_functions(RvCEmitter *emitter, const RvCheckedProgram *program);
void rv_c_function_references(RvCEmitter *emitter, const RvCheckedProgram *program);

#endif
