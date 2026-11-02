#include "../internal.h"

void rv_c_assignment(RvCEmitter *emitter, const RvAstStmt *statement) {
    const RvAstExpr *target = statement->as.assignment.target;
    if (target->kind == RV_AST_INDEX) {
        RvCValue array = rv_c_expression(emitter, target->as.index.base);
        RvCValue index = rv_c_expression(emitter, target->as.index.index);
        rv_c_indent(emitter);
        rv_c_write(emitter, "rv_array_check(rt, rv_t_%zu, rv_t_%zu, ", array.id, index.id);
        rv_c_location(emitter, target->span);
        rv_c_write(emitter, ");\n");
        RvCValue value = rv_c_expression(emitter, statement->as.assignment.value);
        rv_c_indent(emitter);
        rv_c_write(emitter, "rv_array_set(rt, rv_t_%zu, rv_t_%zu, (RvElement){.%s = rv_t_%zu}, ",
                   array.id, index.id, rv_c_element(value.kind), value.id);
        rv_c_location(emitter, target->span);
        rv_c_write(emitter, ");\n");
        return;
    }

    RvCValue value = rv_c_expression(emitter, statement->as.assignment.value);
    RvSymbol *symbol = target->symbol;
    if (target->kind == RV_AST_FIELD) {
        size_t field = target->as.field.resolved->index;
        if (value.kind == RV_TYPE_TEXT) {
            rv_c_indent(emitter);
            rv_c_write(emitter, "rv_text_release(rt, rv_v_%zu.rv_m_%zu);\n", symbol->id, field);
        }
        rv_c_indent(emitter);
        rv_c_write(emitter, "rv_v_%zu.rv_m_%zu = rv_t_%zu;\n", symbol->id, field, value.id);
    } else {
        rv_c_release_named(emitter, symbol->type, "rv_v_", symbol->id);
        rv_c_indent(emitter);
        rv_c_write(emitter, "rv_v_%zu = %srv_t_%zu;\n",
                   symbol->id, value.kind == RV_TYPE_ARRAY ? "*" : "", value.id);
    }
}
