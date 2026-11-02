#include "internal.h"

static bool supported_expression(const RvAstExpr *expression, RvEmitError *error) {
    switch (expression->kind) {
        case RV_AST_INTEGER:
        case RV_AST_REAL:
        case RV_AST_TEXT:
        case RV_AST_BOOL:
        case RV_AST_NAME:
            if (rv_type_primitive(expression->type) || expression->type.kind == RV_TYPE_ARRAY
                || expression->type.kind == RV_TYPE_RECORD || expression->type.kind == RV_TYPE_FILE) {
                return true;
            }
            break;
        case RV_AST_TYPED_ASK:
            return true;
        case RV_AST_GROUP:
            return supported_expression(expression->as.group, error);
        case RV_AST_UNARY:
            return supported_expression(expression->as.unary.operand, error);
        case RV_AST_BINARY:
            return supported_expression(expression->as.binary.left, error)
                && supported_expression(expression->as.binary.right, error);
        case RV_AST_ARRAY:
            for (RvAstExprList *item = expression->as.elements; item != NULL; item = item->next) {
                if (!supported_expression(item->value, error)) {
                    return false;
                }
            }
            return true;
        case RV_AST_RECORD:
            for (RvAstFieldInit *item = expression->as.record.fields; item != NULL; item = item->next) {
                if (!supported_expression(item->value, error)) {
                    return false;
                }
            }
            return true;
        case RV_AST_FIELD:
            return expression->as.field.resolved == NULL
                || supported_expression(expression->as.field.base, error);
        case RV_AST_INDEX:
            return supported_expression(expression->as.index.base, error)
                && supported_expression(expression->as.index.index, error);
        case RV_AST_CALL:
            if (expression->symbol->kind == RV_SYMBOL_FUNCTION
                || expression->symbol->kind == RV_SYMBOL_BUILTIN) {
                for (RvAstExprList *item = expression->as.call.arguments; item != NULL; item = item->next) {
                    if (!supported_expression(item->value, error)) {
                        return false;
                    }
                }
                return true;
            }
            break;
        default:
            break;
    }
    return rv_c_fail(error, RV_EMIT_UNSUPPORTED, expression->span,
                     "native support for this expression is not implemented yet");
}

static bool supported_block(const RvAstStmt *block, RvEmitError *error) {
    for (RvAstSequence *sequence = block->as.statements; sequence != NULL; sequence = sequence->next) {
        const RvAstStmt *statement = sequence->statement;
        if (sequence->alternative != NULL) {
            return rv_c_fail(error, RV_EMIT_ARGUMENT, statement->span, "unresolved syntax reached C emission");
        }
        bool supported = false;
        switch (statement->kind) {
            case RV_AST_BLOCK:
                supported = supported_block(statement, error);
                break;
            case RV_AST_DECLARATION:
                supported = supported_expression(statement->as.declaration.initializer, error);
                break;
            case RV_AST_ASSIGNMENT:
                supported = supported_expression(statement->as.assignment.target, error)
                    && supported_expression(statement->as.assignment.value, error);
                break;
            case RV_AST_EXPRESSION:
                supported = supported_expression(statement->as.expression, error);
                break;
            case RV_AST_GIVE:
                supported = statement->as.expression == NULL
                    || supported_expression(statement->as.expression, error);
                break;
            case RV_AST_CHECK:
                supported = supported_expression(statement->as.check.condition, error)
                    && supported_block(statement->as.check.body, error)
                    && (statement->as.check.otherwise == NULL
                        || supported_block(statement->as.check.otherwise, error));
                break;
            case RV_AST_CHOOSE:
                supported = supported_expression(statement->as.choose.selector, error);
                for (RvAstCase *arm = statement->as.choose.cases;
                     arm != NULL && supported; arm = arm->next) {
                    supported = supported_block(arm->body, error);
                }
                if (supported && statement->as.choose.otherwise != NULL) {
                    supported = supported_block(statement->as.choose.otherwise, error);
                }
                break;
            case RV_AST_RANGE_WALK:
                supported = supported_expression(statement->as.range_walk.begin, error)
                    && supported_expression(statement->as.range_walk.end, error)
                    && supported_block(statement->as.range_walk.body, error);
                break;
            case RV_AST_EACH_WALK:
                supported = supported_expression(statement->as.each_walk.iterable, error)
                    && supported_block(statement->as.each_walk.body, error);
                break;
            case RV_AST_DURING:
                supported = supported_expression(statement->as.during.condition, error)
                    && supported_block(statement->as.during.body, error);
                break;
            default:
                break;
        }
        if (!supported) {
            return rv_c_fail(error, RV_EMIT_UNSUPPORTED, statement->span,
                             "native support for this statement is not implemented yet");
        }
    }
    return true;
}

static bool supported_function(const RvAstDecl *declaration, RvEmitError *error) {
    return supported_block(declaration->as.function.body, error);
}

bool rv_c_supported(const RvCheckedProgram *program, RvEmitError *error) {
    RvEmitError local_error;
    if (error == NULL) {
        error = &local_error;
    }
    *error = (RvEmitError){0};
    if (program == NULL || !program->valid || program->graph.entry == NULL) {
        return rv_c_fail(error, RV_EMIT_ARGUMENT, (RvSpan){0}, "a checked program is required");
    }
    for (size_t index = 0; index < program->graph.modules.length; ++index) {
        RvModuleUnit *unit = rv_module_graph_at(&program->graph, index);
        for (RvAstDecl *declaration = unit->ast.declarations;
             declaration != NULL; declaration = declaration->next) {
            if (declaration->kind == RV_AST_FUNCTION && !supported_function(declaration, error)) {
                return false;
            }
        }
    }
    return supported_block(program->graph.entry->ast.launch->as.function.body, error);
}
