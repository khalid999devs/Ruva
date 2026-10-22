#include "../internal.h"

static size_t argument_count(const RvAstExprList *arguments) {
    size_t count = 0;
    for (const RvAstExprList *item = arguments; item != NULL; item = item->next) {
        ++count;
    }
    return count;
}

static RvType invalid_call(RvChecker *checker, RvSpan span, const char *message) {
    rv_check_fail(checker, RV_CHECK_TYPE, span, message);
    return (RvType){0};
}

static RvType check_user_call(RvChecker *checker, RvAstExpr *expression, RvSymbol *function) {
    RvParameter *parameter = function->parameters;
    RvAstExprList *argument = expression->as.call.arguments;
    while (parameter != NULL && argument != NULL) {
        RvType type = rv_check_expression(checker, argument->value, parameter->type);
        if (type.kind == RV_TYPE_INVALID) {
            return type;
        }
        if (!rv_type_equal(type, parameter->type)) {
            return invalid_call(checker, argument->value->span, "function argument type mismatch");
        }
        parameter = parameter->next;
        argument = argument->next;
    }
    if (parameter != NULL || argument != NULL) {
        return invalid_call(checker, expression->span, "function argument count mismatch");
    }
    return function->type;
}

static RvType check_show(RvChecker *checker, RvAstExpr *expression) {
    for (RvAstExprList *item = expression->as.call.arguments; item != NULL; item = item->next) {
        RvType type = rv_check_expression(checker, item->value, (RvType){0});
        if (type.kind == RV_TYPE_INVALID) {
            return type;
        }
        if (!rv_type_primitive(type)) {
            return invalid_call(checker, item->value->span, "show accepts only primitive values");
        }
    }
    return (RvType){.kind = RV_TYPE_VOID};
}

static RvType check_conversion(RvChecker *checker, RvAstExpr *expression, RvBuiltin builtin,
                                RvType input) {
    bool permitted = false;
    RvTypeKind result = RV_TYPE_INVALID;
    switch (builtin) {
        case RV_BUILTIN_TO_INT:
            permitted = input.kind == RV_TYPE_REAL || input.kind == RV_TYPE_TEXT;
            result = RV_TYPE_INT;
            break;
        case RV_BUILTIN_TO_REAL:
            permitted = input.kind == RV_TYPE_INT || input.kind == RV_TYPE_TEXT;
            result = RV_TYPE_REAL;
            break;
        case RV_BUILTIN_TO_TEXT:
            permitted = input.kind == RV_TYPE_INT || input.kind == RV_TYPE_REAL || input.kind == RV_TYPE_BOOL;
            result = RV_TYPE_TEXT;
            break;
        case RV_BUILTIN_TO_BOOL:
            permitted = input.kind == RV_TYPE_TEXT;
            result = RV_TYPE_BOOL;
            break;
        default:
            break;
    }
    if (!permitted) {
        return invalid_call(checker, expression->span, "conversion does not accept this input type");
    }
    return (RvType){.kind = result};
}

static RvType check_array_call(RvChecker *checker, RvAstExpr *expression,
                                RvBuiltin builtin, RvType first, RvType second) {
    if (first.kind != RV_TYPE_ARRAY) {
        return invalid_call(checker, expression->span, "array built-in requires an array");
    }
    if (builtin == RV_BUILTIN_LENGTH) {
        return (RvType){.kind = RV_TYPE_INT};
    }

    RvAstExpr *array = rv_check_ungroup(expression->as.call.arguments->value);
    if (array->kind != RV_AST_NAME) {
        return invalid_call(checker, array->span, "push and pop require an array binding");
    }
    if (!rv_check_mutation(checker, array->symbol, array->span, true)) {
        return (RvType){0};
    }
    if (builtin == RV_BUILTIN_PUSH) {
        RvType element = {.kind = first.element};
        if (!rv_type_equal(element, second)) {
            return invalid_call(checker, expression->span, "push element type mismatch");
        }
        return (RvType){.kind = RV_TYPE_VOID};
    }
    return (RvType){.kind = first.element};
}

static RvType check_file_call(RvChecker *checker, RvAstExpr *expression,
                               RvBuiltin builtin, RvType first, RvType second) {
    RvTypeKind first_kind = RV_TYPE_TEXT;
    if (builtin == RV_BUILTIN_FILE_READ_FROM || builtin == RV_BUILTIN_FILE_WRITE_TO
        || builtin == RV_BUILTIN_FILE_CLOSE) {
        first_kind = RV_TYPE_FILE;
    }
    bool has_second = builtin == RV_BUILTIN_FILE_WRITE || builtin == RV_BUILTIN_FILE_APPEND
        || builtin == RV_BUILTIN_FILE_OPEN || builtin == RV_BUILTIN_FILE_WRITE_TO;

    if (first.kind != first_kind || (has_second && second.kind != RV_TYPE_TEXT)) {
        return invalid_call(checker, expression->span, "file operation argument type mismatch");
    }
    RvTypeKind result = RV_TYPE_BOOL;
    if (builtin == RV_BUILTIN_FILE_OPEN) {
        result = RV_TYPE_FILE;
    } else if (builtin == RV_BUILTIN_FILE_READ || builtin == RV_BUILTIN_FILE_READ_FROM) {
        result = RV_TYPE_TEXT;
    }
    return (RvType){.kind = result};
}

static RvType check_builtin_call(RvChecker *checker, RvAstExpr *expression, RvBuiltin builtin) {
    if (builtin == RV_BUILTIN_SHOW) {
        return check_show(checker, expression);
    }
    if (builtin == RV_BUILTIN_ASK) {
        return invalid_call(checker, expression->span, "ask requires a primitive type argument");
    }

    size_t expected = 1;
    if (builtin == RV_BUILTIN_PUSH || builtin == RV_BUILTIN_FILE_WRITE
        || builtin == RV_BUILTIN_FILE_APPEND || builtin == RV_BUILTIN_FILE_OPEN
        || builtin == RV_BUILTIN_FILE_WRITE_TO) {
        expected = 2;
    }
    if (argument_count(expression->as.call.arguments) != expected) {
        return invalid_call(checker, expression->span, "built-in argument count mismatch");
    }

    RvType first = rv_check_expression(checker, expression->as.call.arguments->value, (RvType){0});
    if (first.kind == RV_TYPE_INVALID) {
        return first;
    }
    RvType second = {0};
    if (expected == 2) {
        RvType expected_element = {0};
        if (builtin == RV_BUILTIN_PUSH && first.kind == RV_TYPE_ARRAY) {
            expected_element.kind = first.element;
        }
        second = rv_check_expression(
            checker, expression->as.call.arguments->next->value, expected_element
        );
        if (second.kind == RV_TYPE_INVALID) {
            return second;
        }
    }

    if (builtin >= RV_BUILTIN_TO_INT && builtin <= RV_BUILTIN_TO_BOOL) {
        return check_conversion(checker, expression, builtin, first);
    }
    if (builtin >= RV_BUILTIN_PUSH && builtin <= RV_BUILTIN_LENGTH) {
        return check_array_call(checker, expression, builtin, first, second);
    }
    return check_file_call(checker, expression, builtin, first, second);
}

RvType rv_check_call(RvChecker *checker, RvAstExpr *expression) {
    RvAstExpr *callee = rv_check_ungroup(expression->as.call.callee);
    if (callee->kind != RV_AST_NAME) {
        return invalid_call(checker, callee->span, "calls require a named function");
    }

    RvSymbol *function = rv_scope_lookup(checker->scope, callee->as.atom->spelling);
    if (function == NULL) {
        rv_check_fail(checker, RV_CHECK_NAME, callee->span, "unknown function");
        return (RvType){0};
    }
    if (function->kind != RV_SYMBOL_FUNCTION && function->kind != RV_SYMBOL_BUILTIN) {
        return invalid_call(checker, callee->span, "called name is not a function");
    }

    for (RvAstExpr *part = expression->as.call.callee;; part = part->as.group) {
        part->symbol = function;
        part->type.kind = RV_TYPE_FUNCTION;
        if (part->kind != RV_AST_GROUP) {
            break;
        }
    }
    expression->symbol = function;

    if (function->kind == RV_SYMBOL_BUILTIN) {
        return check_builtin_call(checker, expression, function->builtin);
    }
    return check_user_call(checker, expression, function);
}
