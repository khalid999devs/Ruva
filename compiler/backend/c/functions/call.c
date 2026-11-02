#include "../internal.h"

typedef struct Argument {
    RvCValue value;
    struct Argument *next;
} Argument;

static Argument *evaluate_arguments(RvCEmitter *emitter, const RvAstExpr *expression) {
    Argument *arguments = NULL;
    Argument **tail = &arguments;
    for (RvAstExprList *item = expression->as.call.arguments; item != NULL; item = item->next) {
        Argument *argument = rv_c_new(emitter, sizeof(*argument), item->value->span);
        if (argument == NULL) {
            return NULL;
        }
        argument->value = rv_c_expression(emitter, item->value);
        *tail = argument;
        tail = &argument->next;
    }
    return arguments;
}

static void release_arguments(RvCEmitter *emitter, Argument *arguments) {
    for (Argument *argument = arguments; argument != NULL; argument = argument->next) {
        rv_c_release(emitter, argument->value);
    }
}

static RvCValue show(RvCEmitter *emitter, const RvAstExpr *expression, Argument *arguments) {
    for (Argument *argument = arguments; argument != NULL; argument = argument->next) {
        if (argument != arguments) {
            rv_c_indent(emitter);
            rv_c_write(emitter, "rv_print_separator(rt, ");
            rv_c_location(emitter, expression->span);
            rv_c_write(emitter, ");\n");
        }
        const char *printer = "rv_print_text";
        switch (argument->value.kind) {
            case RV_TYPE_INT:
                printer = "rv_print_int";
                break;
            case RV_TYPE_REAL:
                printer = "rv_print_real";
                break;
            case RV_TYPE_BOOL:
                printer = "rv_print_bool";
                break;
            default:
                break;
        }
        rv_c_indent(emitter);
        rv_c_write(emitter, "%s(rt, rv_t_%zu, ", printer, argument->value.id);
        rv_c_location(emitter, expression->span);
        rv_c_write(emitter, ");\n");
    }
    rv_c_indent(emitter);
    rv_c_write(emitter, "rv_print_newline(rt, ");
    rv_c_location(emitter, expression->span);
    rv_c_write(emitter, ");\n");
    release_arguments(emitter, arguments);
    return (RvCValue){.kind = RV_TYPE_VOID};
}

RvCValue rv_c_call(RvCEmitter *emitter, const RvAstExpr *expression) {
    if (expression->symbol->kind == RV_SYMBOL_BUILTIN
        && expression->symbol->builtin >= RV_BUILTIN_PUSH
        && expression->symbol->builtin <= RV_BUILTIN_LENGTH) {
        return rv_c_array_call(emitter, expression);
    }
    Argument *arguments = evaluate_arguments(emitter, expression);
    if (emitter->error->kind != RV_EMIT_OK) {
        return (RvCValue){0};
    }
    if (expression->symbol->kind == RV_SYMBOL_BUILTIN
        && expression->symbol->builtin == RV_BUILTIN_SHOW) {
        return show(emitter, expression, arguments);
    }
    RvCValue result = {.kind = RV_TYPE_VOID};
    if (expression->type.kind == RV_TYPE_VOID) {
        rv_c_indent(emitter);
    } else {
        result = rv_c_new_typed_value(emitter, expression->type);
    }
    if (expression->symbol->kind == RV_SYMBOL_BUILTIN) {
        const char *helper = rv_c_builtin_helper(expression->symbol->builtin, arguments->value.kind);
        rv_c_write(emitter, "%s(rt", helper);
    } else {
        rv_c_write(emitter, "rv_f_%zu(rt", expression->symbol->id);
    }
    for (Argument *argument = arguments; argument != NULL; argument = argument->next) {
        rv_c_write(emitter, ", rv_t_%zu", argument->value.id);
    }
    if (expression->symbol->kind == RV_SYMBOL_BUILTIN) {
        rv_c_write(emitter, ", ");
        rv_c_location(emitter, expression->span);
    }
    rv_c_write(emitter, ");\n");
    release_arguments(emitter, arguments);
    return result;
}

void rv_c_discard(RvCEmitter *emitter, RvCValue value) {
    rv_c_release(emitter, value);
    if (value.kind != RV_TYPE_VOID && value.id != 0) {
        rv_c_indent(emitter);
        rv_c_write(emitter, "(void)rv_t_%zu;\n", value.id);
    }
}
