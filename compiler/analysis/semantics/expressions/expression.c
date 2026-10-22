#include "../internal.h"

#include <string.h>

RvAstExpr *rv_check_ungroup(RvAstExpr *expression) {
    while (expression->kind == RV_AST_GROUP) {
        expression = expression->as.group;
    }
    return expression;
}

bool rv_check_fresh(RvChecker *checker, RvAstExpr *expression, RvType type) {
    RvAstExpr *value = rv_check_ungroup(expression);
    if (type.kind == RV_TYPE_ARRAY && value->kind != RV_AST_ARRAY) {
        return rv_check_fail(checker, RV_CHECK_OWNERSHIP, expression->span,
                             "array initialization and replacement require a fresh literal");
    }
    if (type.kind == RV_TYPE_FILE
        && (value->kind != RV_AST_CALL || value->symbol == NULL
            || value->symbol->builtin != RV_BUILTIN_FILE_OPEN)) {
        return rv_check_fail(checker, RV_CHECK_OWNERSHIP, expression->span,
                             "file initialization and rebinding require a fresh fileOpen call");
    }
    return true;
}

bool rv_check_mutation(RvChecker *checker, RvSymbol *symbol, RvSpan span, bool resize) {
    if (symbol == NULL || !symbol->mutable) {
        return rv_check_fail(checker, RV_CHECK_OWNERSHIP, span, "binding is read-only");
    }
    if (resize) {
        for (RvActiveWalk *walk = checker->walks; walk != NULL; walk = walk->next) {
            if (walk->array == symbol) {
                return rv_check_fail(checker, RV_CHECK_OWNERSHIP, span,
                                     "cannot resize or replace an active array walk target");
            }
        }
    }
    return true;
}

static RvType check_name(RvChecker *checker, RvAstExpr *expression) {
    RvSymbol *symbol = rv_scope_lookup(checker->scope, expression->as.atom->spelling);
    if (symbol == NULL) {
        rv_check_fail(checker, RV_CHECK_NAME, expression->span, "unknown name");
        return (RvType){0};
    }
    if (symbol->kind == RV_SYMBOL_RECORD || symbol->kind == RV_SYMBOL_FUNCTION
        || symbol->kind == RV_SYMBOL_BUILTIN || symbol->kind == RV_SYMBOL_PROJECTION) {
        rv_check_fail(checker, RV_CHECK_TYPE, expression->span, "name is not a first-class value");
        return (RvType){0};
    }
    expression->symbol = symbol;
    return symbol->type;
}

static RvType check_array(RvChecker *checker, RvAstExpr *expression, RvType expected) {
    RvType element = {0};
    if (expected.kind == RV_TYPE_ARRAY) {
        element.kind = expected.element;
    }

    for (RvAstExprList *item = expression->as.elements; item != NULL; item = item->next) {
        RvType actual = rv_check_expression(checker, item->value, element);
        if (actual.kind == RV_TYPE_INVALID) {
            return actual;
        }
        if (!rv_type_primitive(actual)) {
            rv_check_fail(checker, RV_CHECK_TYPE, item->value->span, "array elements must be primitive values");
            return (RvType){0};
        }
        if (element.kind == RV_TYPE_INVALID) {
            element = actual;
        } else if (!rv_type_equal(element, actual)) {
            rv_check_fail(checker, RV_CHECK_TYPE, item->value->span, "array element type mismatch");
            return (RvType){0};
        }
    }
    if (element.kind == RV_TYPE_INVALID) {
        rv_check_fail(checker, RV_CHECK_TYPE, expression->span, "empty array needs an expected array type");
        return (RvType){0};
    }
    return (RvType){.kind = RV_TYPE_ARRAY, .element = element.kind};
}

static RvType check_field(RvChecker *checker, RvAstExpr *expression) {
    RvAstExpr *base = expression->as.field.base;
    RvAstExpr *name = rv_check_ungroup(base);
    if (name->kind == RV_AST_NAME) {
        RvSymbol *symbol = rv_scope_lookup(checker->scope, name->as.atom->spelling);
        if (symbol != NULL && symbol->kind == RV_SYMBOL_PROJECTION) {
            const char *field = expression->as.field.name->spelling;
            if (strcmp(field, "key") != 0 && strcmp(field, "value") != 0) {
                rv_check_fail(checker, RV_CHECK_TYPE, expression->as.field.name->span,
                              "record walk binding exposes only key and value");
                return (RvType){0};
            }
            for (RvAstExpr *part = base;; part = part->as.group) {
                part->symbol = symbol;
                part->type = symbol->type;
                if (part->kind != RV_AST_GROUP) {
                    break;
                }
            }
            expression->symbol = symbol;
            return (RvType){.kind = RV_TYPE_TEXT};
        }
    }

    RvType type = rv_check_expression(checker, base, (RvType){0});
    if (type.kind == RV_TYPE_INVALID) {
        return type;
    }
    if (type.kind != RV_TYPE_RECORD) {
        rv_check_fail(checker, RV_CHECK_TYPE, base->span, "field access requires a record");
        return (RvType){0};
    }
    RvField *field = rv_check_field(type.record, expression->as.field.name->spelling);
    if (field == NULL) {
        rv_check_fail(checker, RV_CHECK_TYPE, expression->as.field.name->span, "unknown record field");
        return (RvType){0};
    }
    expression->as.field.resolved = field;
    expression->symbol = base->symbol;
    return field->type;
}

static RvType check_unary(RvChecker *checker, RvAstExpr *expression) {
    RvTokenKind operator = expression->as.unary.operator->kind;
    RvAstExpr *operand = expression->as.unary.operand;
    RvAstExpr *literal = rv_check_ungroup(operand);
    RvType type;

    if (operator == RV_TOKEN_MINUS && literal->kind == RV_AST_INTEGER) {
        if (!rv_check_integer(checker, literal, true)) {
            return (RvType){0};
        }
        type = literal->type;
        for (RvAstExpr *group = operand; group != literal; group = group->as.group) {
            group->type = type;
        }
    } else {
        type = rv_check_expression(checker, operand, (RvType){0});
    }
    if (type.kind == RV_TYPE_INVALID) {
        return type;
    }
    if ((operator == RV_TOKEN_BANG && type.kind != RV_TYPE_BOOL)
        || (operator == RV_TOKEN_MINUS && !rv_type_numeric(type))) {
        rv_check_fail(checker, RV_CHECK_TYPE, expression->span, "invalid unary operand type");
        return (RvType){0};
    }
    return type;
}

static RvType check_binary(RvChecker *checker, RvAstExpr *expression) {
    RvType left = rv_check_expression(checker, expression->as.binary.left, (RvType){0});
    RvType right = rv_check_expression(checker, expression->as.binary.right, (RvType){0});
    if (left.kind == RV_TYPE_INVALID || right.kind == RV_TYPE_INVALID) {
        return (RvType){0};
    }
    if (!rv_type_equal(left, right)) {
        rv_check_fail(checker, RV_CHECK_TYPE, expression->span, "operator operands must have the same type");
        return (RvType){0};
    }

    RvTokenKind operator = expression->as.binary.operator->kind;
    bool valid = false;
    RvType result = left;
    switch (operator) {
        case RV_TOKEN_PLUS:
            valid = rv_type_numeric(left) || left.kind == RV_TYPE_TEXT;
            break;
        case RV_TOKEN_MINUS:
        case RV_TOKEN_STAR:
        case RV_TOKEN_SLASH:
            valid = rv_type_numeric(left);
            break;
        case RV_TOKEN_PERCENT:
            valid = left.kind == RV_TYPE_INT;
            break;
        case RV_TOKEN_EQUAL_EQUAL:
        case RV_TOKEN_BANG_EQUAL:
            valid = rv_type_primitive(left);
            result.kind = RV_TYPE_BOOL;
            break;
        case RV_TOKEN_LESS:
        case RV_TOKEN_LESS_EQUAL:
        case RV_TOKEN_GREATER:
        case RV_TOKEN_GREATER_EQUAL:
            valid = rv_type_numeric(left);
            result.kind = RV_TYPE_BOOL;
            break;
        case RV_TOKEN_AND_AND:
        case RV_TOKEN_OR_OR:
            valid = left.kind == RV_TYPE_BOOL;
            break;
        default:
            break;
    }
    if (!valid) {
        rv_check_fail(checker, RV_CHECK_TYPE, expression->span, "operator does not accept these operand types");
        return (RvType){0};
    }
    return result;
}

RvType rv_check_expression(RvChecker *checker, RvAstExpr *expression, RvType expected) {
    if (!rv_check_step(checker, expression->span)) {
        return (RvType){0};
    }
    if (checker->depth >= 256) {
        rv_check_fail(checker, RV_CHECK_LIMIT, expression->span, "semantic nesting limit exceeded");
        return (RvType){0};
    }
    ++checker->depth;
    RvType type = {0};

    switch (expression->kind) {
        case RV_AST_INTEGER:
            if (rv_check_integer(checker, expression, false)) {
                type.kind = RV_TYPE_INT;
            }
            break;
        case RV_AST_REAL:
            if (rv_check_real(checker, expression)) {
                type.kind = RV_TYPE_REAL;
            }
            break;
        case RV_AST_BOOL:
            type.kind = RV_TYPE_BOOL;
            break;
        case RV_AST_TEXT:
            type.kind = RV_TYPE_TEXT;
            break;
        case RV_AST_NAME:
            type = check_name(checker, expression);
            break;
        case RV_AST_GROUP:
            type = rv_check_expression(checker, expression->as.group, expected);
            expression->symbol = expression->as.group->symbol;
            break;
        case RV_AST_UNARY:
            type = check_unary(checker, expression);
            break;
        case RV_AST_BINARY:
            type = check_binary(checker, expression);
            break;
        case RV_AST_CALL:
            type = rv_check_call(checker, expression);
            break;
        case RV_AST_INDEX: {
            RvType array = rv_check_expression(checker, expression->as.index.base, (RvType){0});
            RvType index = rv_check_expression(checker, expression->as.index.index, (RvType){0});
            if (array.kind == RV_TYPE_ARRAY && index.kind == RV_TYPE_INT) {
                type.kind = array.element;
                expression->symbol = expression->as.index.base->symbol;
            } else {
                rv_check_fail(checker, RV_CHECK_TYPE, expression->span,
                              "indexing requires an array and an int index");
            }
            break;
        }
        case RV_AST_FIELD:
            type = check_field(checker, expression);
            break;
        case RV_AST_ARRAY:
            type = check_array(checker, expression, expected);
            break;
        case RV_AST_RECORD:
            type = rv_check_record(checker, expression);
            break;
        case RV_AST_TYPED_ASK:
            type = rv_check_type(checker, expression->as.ask_type);
            expression->symbol = rv_scope_local(checker->builtins, "ask");
            break;
    }

    expression->type = type;
    --checker->depth;
    return type;
}

bool rv_check_compatible(RvChecker *checker, const RvAstExpr *expression) {
    if (!rv_check_step(checker, expression->span)) {
        return false;
    }
    switch (expression->kind) {
        case RV_AST_NAME: {
            RvSymbol *symbol = rv_scope_lookup(checker->scope, expression->as.atom->spelling);
            if (symbol == NULL) {
                return rv_check_fail(checker, RV_CHECK_NAME, expression->span, "unknown name");
            }
            if (symbol->kind == RV_SYMBOL_RECORD) {
                return rv_check_fail(checker, RV_CHECK_TYPE, expression->span, "record type is not a value");
            }
            return true;
        }
        case RV_AST_RECORD: {
            RvSymbol *symbol = rv_scope_lookup(checker->scope, expression->as.record.name->spelling);
            if (symbol == NULL || symbol->kind != RV_SYMBOL_RECORD) {
                return rv_check_fail(checker, RV_CHECK_NAME, expression->as.record.name->span,
                                     "constructor name does not denote a record type");
            }
            for (RvAstFieldInit *field = expression->as.record.fields; field != NULL; field = field->next) {
                if (!rv_check_compatible(checker, field->value)) {
                    return false;
                }
            }
            return true;
        }
        case RV_AST_GROUP:
            return rv_check_compatible(checker, expression->as.group);
        case RV_AST_UNARY:
            return rv_check_compatible(checker, expression->as.unary.operand);
        case RV_AST_BINARY:
            return rv_check_compatible(checker, expression->as.binary.left)
                && rv_check_compatible(checker, expression->as.binary.right);
        case RV_AST_INDEX:
            return rv_check_compatible(checker, expression->as.index.base)
                && rv_check_compatible(checker, expression->as.index.index);
        case RV_AST_FIELD:
            return rv_check_compatible(checker, expression->as.field.base);
        case RV_AST_CALL:
        case RV_AST_ARRAY: {
            const RvAstExprList *items = NULL;
            if (expression->kind == RV_AST_CALL) {
                if (!rv_check_compatible(checker, expression->as.call.callee)) {
                    return false;
                }
                items = expression->as.call.arguments;
            } else {
                items = expression->as.elements;
            }
            for (const RvAstExprList *item = items; item != NULL; item = item->next) {
                if (!rv_check_compatible(checker, item->value)) {
                    return false;
                }
            }
            return true;
        }
        default:
            return true;
    }
}
