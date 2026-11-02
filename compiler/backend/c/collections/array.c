#include "../internal.h"

const char *rv_c_element(RvTypeKind kind) {
    switch (kind) {
        case RV_TYPE_INT:
            return "integer";
        case RV_TYPE_REAL:
            return "real";
        case RV_TYPE_BOOL:
            return "boolean";
        default:
            return "text";
    }
}

RvCValue rv_c_array(RvCEmitter *emitter, const RvAstExpr *expression) {
    size_t id = ++emitter->next_temporary;
    rv_c_indent(emitter);
    rv_c_write(emitter, "RvArray rv_a_%zu = {.owns_text = %s};\n",
               id, expression->type.element == RV_TYPE_TEXT ? "true" : "false");
    RvCValue value = rv_c_new_value(emitter, RV_TYPE_ARRAY);
    rv_c_write(emitter, "&rv_a_%zu;\n", id);
    for (RvAstExprList *item = expression->as.elements; item != NULL; item = item->next) {
        RvCValue element = rv_c_expression(emitter, item->value);
        rv_c_indent(emitter);
        rv_c_write(emitter, "rv_array_push(rt, rv_t_%zu, (RvElement){.%s = rv_t_%zu}, ",
                   value.id, rv_c_element(element.kind), element.id);
        rv_c_location(emitter, item->value->span);
        rv_c_write(emitter, ");\n");
    }
    return value;
}

RvCValue rv_c_array_call(RvCEmitter *emitter, const RvAstExpr *expression) {
    RvAstExprList *arguments = expression->as.call.arguments;
    RvCValue array = rv_c_expression(emitter, arguments->value);
    RvBuiltin builtin = expression->symbol->builtin;
    RvCValue value = {.kind = RV_TYPE_VOID};
    if (builtin == RV_BUILTIN_PUSH) {
        RvCValue element = rv_c_expression(emitter, arguments->next->value);
        rv_c_indent(emitter);
        rv_c_write(emitter, "rv_array_push(rt, rv_t_%zu, (RvElement){.%s = rv_t_%zu}, ",
                   array.id, rv_c_element(element.kind), element.id);
        rv_c_location(emitter, expression->span);
        rv_c_write(emitter, ");\n");
    } else {
        value = rv_c_new_value(emitter, expression->type.kind);
        rv_c_write(emitter, "%s(rt, rv_t_%zu, ",
                   builtin == RV_BUILTIN_POP ? "rv_array_pop" : "rv_array_length", array.id);
        rv_c_location(emitter, expression->span);
        rv_c_write(emitter, ")");
        if (builtin == RV_BUILTIN_POP) {
            rv_c_write(emitter, ".%s", rv_c_element(value.kind));
        }
        rv_c_write(emitter, ";\n");
    }
    rv_c_release(emitter, array);
    return value;
}

RvCValue rv_c_index(RvCEmitter *emitter, const RvAstExpr *expression) {
    RvCValue array = rv_c_expression(emitter, expression->as.index.base);
    RvCValue index = rv_c_expression(emitter, expression->as.index.index);
    RvCValue value = rv_c_new_value(emitter, expression->type.kind);
    rv_c_write(emitter, "rv_array_get(rt, rv_t_%zu, rv_t_%zu, ", array.id, index.id);
    rv_c_location(emitter, expression->span);
    rv_c_write(emitter, ").%s;\n", rv_c_element(value.kind));
    rv_c_release(emitter, array);
    return value;
}
