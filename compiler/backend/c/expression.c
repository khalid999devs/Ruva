#include "internal.h"
#include "compiler/common/host/numeric.h"

#include <inttypes.h>

RvCValue rv_c_new_value(RvCEmitter *emitter, RvTypeKind kind) {
    return rv_c_new_typed_value(emitter, (RvType){.kind = kind});
}

void rv_c_release(RvCEmitter *emitter, RvCValue value) {
    if (!value.borrowed) {
        RvType type = {.kind = value.kind, .record = value.record};
        rv_c_release_named(emitter, type, "rv_t_", value.id);
    }
}

static RvCValue literal(RvCEmitter *emitter, const RvAstExpr *expression) {
    RvCValue value = rv_c_new_value(emitter, expression->type.kind);
    switch (expression->kind) {
        case RV_AST_INTEGER:
            rv_c_write(emitter, "INT32_C(%" PRId64 ")", expression->number.integer);
            break;
        case RV_AST_REAL: {
            char spelling[64];
            int length = rv_host_format_real(spelling, sizeof(spelling), "%a", expression->number.real);
            if (length < 0 || (size_t)length >= sizeof(spelling)) {
                rv_c_fail(emitter->error, RV_EMIT_IO, expression->span, "cannot format real literal");
            } else {
                rv_c_write(emitter, "%s", spelling);
            }
            break;
        }
        case RV_AST_BOOL:
            rv_c_write(emitter, "%s", expression->as.atom->kind == RV_TOKEN_TRUE ? "true" : "false");
            break;
        case RV_AST_TEXT:
            rv_c_write(emitter, "rv_text_create(rt, ");
            rv_c_string(emitter, expression->as.atom->text, expression->as.atom->text_length);
            rv_c_write(emitter, ", %zu, ", expression->as.atom->text_length);
            rv_c_location(emitter, expression->span);
            rv_c_write(emitter, ")");
            break;
        default:
            break;
    }
    rv_c_write(emitter, ";\n");
    return value;
}

static RvCValue name_value(RvCEmitter *emitter, const RvAstExpr *expression) {
    RvSymbol *symbol = expression->symbol;
    if (symbol->kind == RV_SYMBOL_CONSTANT) {
        return rv_c_expression(emitter, symbol->declaration->as.constant.initializer);
    }
    RvCValue value = rv_c_new_typed_value(emitter, expression->type);
    if (value.kind == RV_TYPE_ARRAY) {
        value.borrowed = true;
        rv_c_write(emitter, "&rv_v_%zu;\n", symbol->id);
    } else if (value.kind == RV_TYPE_TEXT) {
        rv_c_write(emitter, "rv_text_retain(rt, rv_v_%zu, ", symbol->id);
        rv_c_location(emitter, expression->span);
        rv_c_write(emitter, ");\n");
    } else {
        rv_c_write(emitter, "rv_v_%zu;\n", symbol->id);
    }
    rv_c_retain_record(emitter, value, expression->span);
    return value;
}

static RvCValue unary(RvCEmitter *emitter, const RvAstExpr *expression) {
    const RvAstExpr *operand = expression->as.unary.operand;
    const RvAstExpr *ungrouped = operand;
    while (ungrouped->kind == RV_AST_GROUP) {
        ungrouped = ungrouped->as.group;
    }
    RvTokenKind operator = expression->as.unary.operator->kind;
    if (operator == RV_TOKEN_MINUS && ungrouped->kind == RV_AST_INTEGER
        && ungrouped->number.integer == INT64_C(2147483648)) {
        RvCValue value = rv_c_new_value(emitter, RV_TYPE_INT);
        rv_c_write(emitter, "INT32_MIN;\n");
        return value;
    }
    RvCValue input = rv_c_expression(emitter, operand);
    RvCValue value = rv_c_new_value(emitter, expression->type.kind);
    if (operator == RV_TOKEN_MINUS && input.kind == RV_TYPE_INT) {
        rv_c_write(emitter, "rv_int_negate(rt, rv_t_%zu, ", input.id);
        rv_c_location(emitter, expression->span);
        rv_c_write(emitter, ");\n");
    } else {
        rv_c_write(emitter, "%s rv_t_%zu;\n", rv_token_kind_name(operator), input.id);
    }
    return value;
}

static const char *numeric_helper(RvTokenKind operator, RvTypeKind kind) {
    if (kind == RV_TYPE_REAL) {
        return operator == RV_TOKEN_SLASH ? "rv_real_divide" : NULL;
    }
    if (kind != RV_TYPE_INT) {
        return NULL;
    }
    switch (operator) {
        case RV_TOKEN_PLUS:
            return "rv_int_add";
        case RV_TOKEN_MINUS:
            return "rv_int_subtract";
        case RV_TOKEN_STAR:
            return "rv_int_multiply";
        case RV_TOKEN_SLASH:
            return "rv_int_divide";
        case RV_TOKEN_PERCENT:
            return "rv_int_remainder";
        default:
            return NULL;
    }
}

static RvCValue binary(RvCEmitter *emitter, const RvAstExpr *expression) {
    RvTokenKind operator = expression->as.binary.operator->kind;
    RvCValue left = rv_c_expression(emitter, expression->as.binary.left);
    if (operator == RV_TOKEN_AND_AND || operator == RV_TOKEN_OR_OR) {
        RvCValue value = rv_c_new_value(emitter, RV_TYPE_BOOL);
        rv_c_write(emitter, "rv_t_%zu;\n", left.id);
        rv_c_indent(emitter);
        rv_c_write(emitter, "if (%srv_t_%zu) {\n", operator == RV_TOKEN_OR_OR ? "!" : "", value.id);
        ++emitter->indentation;
        RvCValue right = rv_c_expression(emitter, expression->as.binary.right);
        rv_c_indent(emitter);
        rv_c_write(emitter, "rv_t_%zu = rv_t_%zu;\n", value.id, right.id);
        --emitter->indentation;
        rv_c_indent(emitter);
        rv_c_write(emitter, "}\n");
        return value;
    }

    RvCValue right = rv_c_expression(emitter, expression->as.binary.right);
    RvCValue value = rv_c_new_value(emitter, expression->type.kind);
    const char *helper = numeric_helper(operator, left.kind);
    if (left.kind == RV_TYPE_TEXT) {
        if (operator == RV_TOKEN_PLUS) {
            rv_c_write(emitter, "rv_text_concat(rt, rv_t_%zu, rv_t_%zu, ", left.id, right.id);
            rv_c_location(emitter, expression->span);
            rv_c_write(emitter, ");\n");
        } else {
            rv_c_write(emitter, "%srv_text_equal(rv_t_%zu, rv_t_%zu);\n",
                       operator == RV_TOKEN_BANG_EQUAL ? "!" : "", left.id, right.id);
        }
    } else if (helper != NULL) {
        rv_c_write(emitter, "%s(rt, rv_t_%zu, rv_t_%zu, ", helper, left.id, right.id);
        rv_c_location(emitter, expression->span);
        rv_c_write(emitter, ");\n");
    } else {
        rv_c_write(emitter, "rv_t_%zu %s rv_t_%zu;\n", left.id, rv_token_kind_name(operator), right.id);
    }
    rv_c_release(emitter, left);
    rv_c_release(emitter, right);
    return value;
}

RvCValue rv_c_expression(RvCEmitter *emitter, const RvAstExpr *expression) {
    emitter->current_span = expression->span;
    switch (expression->kind) {
        case RV_AST_INTEGER:
        case RV_AST_REAL:
        case RV_AST_BOOL:
        case RV_AST_TEXT:
            return literal(emitter, expression);
        case RV_AST_NAME:
            return name_value(emitter, expression);
        case RV_AST_GROUP:
            return rv_c_expression(emitter, expression->as.group);
        case RV_AST_UNARY:
            return unary(emitter, expression);
        case RV_AST_BINARY:
            return binary(emitter, expression);
        case RV_AST_ARRAY:
            return rv_c_array(emitter, expression);
        case RV_AST_RECORD:
            return rv_c_record(emitter, expression);
        case RV_AST_FIELD:
            return rv_c_field(emitter, expression);
        case RV_AST_INDEX:
            return rv_c_index(emitter, expression);
        case RV_AST_TYPED_ASK:
            return rv_c_ask(emitter, expression);
        case RV_AST_CALL:
            return rv_c_call(emitter, expression);
        default:
            rv_c_fail(emitter->error, RV_EMIT_UNSUPPORTED, expression->span, "unsupported native expression");
            return (RvCValue){0};
    }
}
