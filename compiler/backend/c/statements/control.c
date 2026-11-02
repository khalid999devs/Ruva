#include "../internal.h"

#include <inttypes.h>

static bool check(RvCEmitter *emitter, const RvAstStmt *statement) {
    RvCValue condition = rv_c_expression(emitter, statement->as.check.condition);
    rv_c_indent(emitter);
    rv_c_write(emitter, "if (rv_t_%zu)\n", condition.id);
    bool body_returns = rv_c_block(emitter, statement->as.check.body);
    bool otherwise_returns = false;
    if (statement->as.check.otherwise != NULL) {
        rv_c_indent(emitter);
        rv_c_write(emitter, "else\n");
        otherwise_returns = rv_c_block(emitter, statement->as.check.otherwise);
    }
    return body_returns && otherwise_returns;
}

static bool choose(RvCEmitter *emitter, const RvAstStmt *statement) {
    RvCScope *scope = rv_c_scope_begin(emitter, statement->span);
    if (scope == NULL) {
        return false;
    }
    RvCValue selector = rv_c_expression(emitter, statement->as.choose.selector);
    rv_c_own_temporary(emitter, selector, statement->span);
    rv_c_indent(emitter);
    rv_c_write(emitter, "(void)rv_t_%zu;\n", selector.id);
    bool first = true;
    bool all_return = true;
    for (RvAstCase *arm = statement->as.choose.cases; arm != NULL; arm = arm->next) {
        rv_c_indent(emitter);
        rv_c_write(emitter, "%sif (", first ? "" : "else ");
        if (selector.kind == RV_TYPE_INT) {
            rv_c_write(emitter, "rv_t_%zu == INT32_C(%" PRId64 ")", selector.id, arm->integer);
        } else {
            rv_c_write(emitter, "rv_text_equal_bytes(rv_t_%zu, ", selector.id);
            rv_c_string(emitter, arm->label->text, arm->label->text_length);
            rv_c_write(emitter, ", %zu)", arm->label->text_length);
        }
        rv_c_write(emitter, ")\n");
        bool arm_returns = rv_c_block(emitter, arm->body);
        all_return = all_return && arm_returns;
        first = false;
    }
    bool otherwise_returns = false;
    if (statement->as.choose.otherwise != NULL) {
        if (!first) {
            rv_c_indent(emitter);
            rv_c_write(emitter, "else\n");
        }
        otherwise_returns = rv_c_block(emitter, statement->as.choose.otherwise);
    }
    bool returned = all_return && otherwise_returns;
    rv_c_scope_end(emitter, scope, returned);
    return returned;
}

static void range_walk(RvCEmitter *emitter, const RvAstStmt *statement) {
    RvCScope *scope = rv_c_scope_begin(emitter, statement->span);
    if (scope == NULL) {
        return;
    }
    RvCValue begin = rv_c_expression(emitter, statement->as.range_walk.begin);
    RvCValue end = rv_c_expression(emitter, statement->as.range_walk.end);
    size_t binding = statement->as.range_walk.binding->id;
    rv_c_indent(emitter);
    rv_c_write(emitter,
        "for (int32_t rv_v_%zu = rv_t_%zu; rv_v_%zu < rv_t_%zu;\n",
        binding, begin.id, binding, end.id);
    rv_c_indent(emitter);
    rv_c_write(emitter, "     rv_v_%zu = rv_int_add(rt, rv_v_%zu, INT32_C(1), ", binding, binding);
    rv_c_location(emitter, statement->span);
    rv_c_write(emitter, "))\n");
    rv_c_block(emitter, statement->as.range_walk.body);
    rv_c_scope_end(emitter, scope, false);
}

static void during(RvCEmitter *emitter, const RvAstStmt *statement) {
    RvCScope *scope = rv_c_scope_begin(emitter, statement->span);
    if (scope == NULL) {
        return;
    }
    rv_c_indent(emitter);
    rv_c_write(emitter, "for (;;) {\n");
    ++emitter->indentation;
    RvCValue condition = rv_c_expression(emitter, statement->as.during.condition);
    rv_c_indent(emitter);
    rv_c_write(emitter, "if (!rv_t_%zu) {\n", condition.id);
    ++emitter->indentation;
    rv_c_indent(emitter);
    rv_c_write(emitter, "break;\n");
    --emitter->indentation;
    rv_c_indent(emitter);
    rv_c_write(emitter, "}\n");
    rv_c_block(emitter, statement->as.during.body);
    --emitter->indentation;
    rv_c_indent(emitter);
    rv_c_write(emitter, "}\n");
    rv_c_scope_end(emitter, scope, false);
}

bool rv_c_control(RvCEmitter *emitter, const RvAstStmt *statement) {
    switch (statement->kind) {
        case RV_AST_CHECK:
            return check(emitter, statement);
        case RV_AST_CHOOSE:
            return choose(emitter, statement);
        case RV_AST_RANGE_WALK:
            range_walk(emitter, statement);
            return false;
        case RV_AST_EACH_WALK:
            rv_c_each_walk(emitter, statement);
            return false;
        case RV_AST_DURING:
            during(emitter, statement);
            return false;
        default:
            rv_c_fail(emitter->error, RV_EMIT_UNSUPPORTED, statement->span,
                       "unsupported native control statement");
            return false;
    }
}
