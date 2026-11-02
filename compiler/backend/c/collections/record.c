#include "../internal.h"

void rv_c_records(RvCEmitter *emitter, const RvCheckedProgram *program) {
    for (size_t index = 0; index < program->symbols.length; ++index) {
        const RvSymbol *symbol = rv_checked_symbol_at(program, index);
        if (symbol->kind != RV_SYMBOL_RECORD) {
            continue;
        }
        emitter->current_span = symbol->token->span;
        rv_c_write(emitter, "typedef struct {\n");
        for (RvField *field = symbol->fields; field != NULL; field = field->next) {
            rv_c_write(emitter, "    %s rv_m_%zu;\n", rv_c_type(field->type.kind), field->index);
        }
        if (symbol->fields == NULL) {
            rv_c_write(emitter, "    unsigned char rv_empty;\n");
        }
        rv_c_write(emitter, "} rv_r_%zu;\n\n", symbol->id);
    }
}

RvCValue rv_c_record(RvCEmitter *emitter, const RvAstExpr *expression) {
    RvCValue value = rv_c_new_typed_value(emitter, expression->type);
    rv_c_write(emitter, "{0};\n");
    for (RvAstFieldInit *item = expression->as.record.fields; item != NULL; item = item->next) {
        RvCValue field = rv_c_expression(emitter, item->value);
        rv_c_indent(emitter);
        rv_c_write(emitter, "rv_t_%zu.rv_m_%zu = rv_t_%zu;\n", value.id, item->field->index, field.id);
    }
    return value;
}

RvCValue rv_c_field(RvCEmitter *emitter, const RvAstExpr *expression) {
    if (expression->as.field.resolved == NULL) {
        RvCProjection *projection = emitter->projection;
        while (projection != NULL && projection->symbol != expression->symbol) {
            projection = projection->parent;
        }
        if (projection == NULL) {
            rv_c_fail(emitter->error, RV_EMIT_ARGUMENT, expression->span, "missing field walk projection");
            return (RvCValue){0};
        }
        const RvToken *name = expression->as.field.name;
        RvCValue source = name->length == 3 ? projection->key : projection->value;
        RvCValue value = rv_c_new_value(emitter, RV_TYPE_TEXT);
        rv_c_write(emitter, "rv_text_retain(rt, rv_t_%zu, ", source.id);
        rv_c_location(emitter, expression->span);
        rv_c_write(emitter, ");\n");
        return value;
    }
    RvCValue base = rv_c_expression(emitter, expression->as.field.base);
    RvCValue value = rv_c_new_value(emitter, expression->type.kind);
    if (value.kind == RV_TYPE_TEXT) {
        rv_c_write(emitter, "rv_text_retain(rt, ");
    }
    rv_c_write(emitter, "rv_t_%zu.rv_m_%zu", base.id, expression->as.field.resolved->index);
    if (value.kind == RV_TYPE_TEXT) {
        rv_c_write(emitter, ", ");
        rv_c_location(emitter, expression->span);
        rv_c_write(emitter, ")");
    }
    rv_c_write(emitter, ";\n");
    rv_c_release(emitter, base);
    return value;
}
