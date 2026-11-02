#include "../internal.h"

static void cleanup_scope(RvCEmitter *emitter, const RvCScope *scope) {
    for (RvCLocal *local = scope->locals; local != NULL; local = local->next) {
        if (local->symbol == NULL) {
            rv_c_release(emitter, local->temporary);
        } else {
            rv_c_release_named(emitter, local->symbol->type, "rv_v_", local->symbol->id);
        }
    }
}

RvCScope *rv_c_scope_begin(RvCEmitter *emitter, RvSpan span) {
    emitter->current_span = span;
    RvCScope *scope = rv_c_new(emitter, sizeof(*scope), span);
    if (scope == NULL) {
        return NULL;
    }
    scope->parent = emitter->scope;
    emitter->scope = scope;
    rv_c_indent(emitter);
    rv_c_write(emitter, "{\n");
    ++emitter->indentation;
    return scope;
}

void rv_c_scope_end(RvCEmitter *emitter, RvCScope *scope, bool returned) {
    if (!returned) {
        cleanup_scope(emitter, scope);
    }
    --emitter->indentation;
    rv_c_indent(emitter);
    rv_c_write(emitter, "}\n");
    emitter->scope = scope->parent;
}

void rv_c_own_temporary(RvCEmitter *emitter, RvCValue value, RvSpan span) {
    if (value.borrowed || (value.kind != RV_TYPE_TEXT && value.kind != RV_TYPE_RECORD
        && value.kind != RV_TYPE_ARRAY)) {
        return;
    }
    RvCLocal *local = rv_c_new(emitter, sizeof(*local), span);
    if (local != NULL) {
        local->temporary = value;
        local->next = emitter->scope->locals;
        emitter->scope->locals = local;
    }
}

void rv_c_own_local(RvCEmitter *emitter, RvSymbol *symbol, RvSpan span) {
    RvCLocal *local = rv_c_new(emitter, sizeof(*local), span);
    if (local != NULL) {
        local->symbol = symbol;
        local->next = emitter->scope->locals;
        emitter->scope->locals = local;
    }
}

static void declaration(RvCEmitter *emitter, const RvAstBinding *binding) {
    RvCValue value = rv_c_expression(emitter, binding->initializer);
    rv_c_indent(emitter);
    if (value.kind == RV_TYPE_ARRAY) {
        rv_c_write(emitter, "RvArray rv_v_%zu = *rv_t_%zu;\n", binding->symbol->id, value.id);
    } else {
        rv_c_write_type(emitter, binding->symbol->type);
        rv_c_write(emitter, " rv_v_%zu = rv_t_%zu;\n", binding->symbol->id, value.id);
    }
    rv_c_indent(emitter);
    rv_c_write(emitter, "(void)rv_v_%zu;\n", binding->symbol->id);
    rv_c_own_local(emitter, binding->symbol, binding->name->span);
}

static void give(RvCEmitter *emitter, const RvAstStmt *statement) {
    RvCValue value = {0};
    if (statement->as.expression != NULL) {
        value = rv_c_expression(emitter, statement->as.expression);
    }
    for (RvCScope *scope = emitter->scope; scope != NULL; scope = scope->parent) {
        cleanup_scope(emitter, scope);
    }
    rv_c_indent(emitter);
    if (statement->as.expression == NULL) {
        rv_c_write(emitter, "return;\n");
    } else {
        rv_c_write(emitter, "return rv_t_%zu;\n", value.id);
    }
}

bool rv_c_block(RvCEmitter *emitter, const RvAstStmt *block) {
    RvCScope *scope = rv_c_scope_begin(emitter, block->span);
    if (scope == NULL) {
        return false;
    }
    if (scope->parent == NULL) {
        rv_c_indent(emitter);
        rv_c_write(emitter, "(void)rt;\n");
        for (RvParameter *parameter = emitter->function->parameters;
             parameter != NULL; parameter = parameter->next) {
            rv_c_indent(emitter);
            rv_c_write(emitter, "(void)rv_v_%zu;\n", parameter->binding->id);
        }
    }
    bool returned = false;
    for (RvAstSequence *sequence = block->as.statements;
         sequence != NULL && !returned && emitter->error->kind == RV_EMIT_OK;
         sequence = sequence->next) {
        const RvAstStmt *statement = sequence->statement;
        switch (statement->kind) {
            case RV_AST_BLOCK:
                returned = rv_c_block(emitter, statement);
                break;
            case RV_AST_DECLARATION:
                declaration(emitter, &statement->as.declaration);
                break;
            case RV_AST_ASSIGNMENT:
                rv_c_assignment(emitter, statement);
                break;
            case RV_AST_EXPRESSION:
                rv_c_discard(emitter, rv_c_expression(emitter, statement->as.expression));
                break;
            case RV_AST_GIVE:
                give(emitter, statement);
                returned = true;
                break;
            default:
                returned = rv_c_control(emitter, statement);
                break;
        }
    }
    rv_c_scope_end(emitter, scope, returned);
    return returned;
}
