#include "internal.h"

#include <string.h>

static bool check_statement(RvChecker *checker, RvAstStmt *statement, bool *returns);

static bool header_compatible(RvChecker *checker, const RvAstStmt *statement) {
    switch (statement->kind) {
        case RV_AST_CHECK:
            return rv_check_compatible(checker, statement->as.check.condition);
        case RV_AST_CHOOSE:
            return rv_check_compatible(checker, statement->as.choose.selector);
        case RV_AST_DURING:
            return rv_check_compatible(checker, statement->as.during.condition);
        case RV_AST_EACH_WALK:
            return rv_check_compatible(checker, statement->as.each_walk.iterable);
        case RV_AST_RANGE_WALK:
            return rv_check_compatible(checker, statement->as.range_walk.begin)
                && rv_check_compatible(checker, statement->as.range_walk.end);
        default:
            return rv_check_fail(checker, RV_CHECK_TYPE, statement->span, "invalid deferred header");
    }
}

static bool select_sequence(RvChecker *checker, RvAstSequence *sequence) {
    if (sequence->alternative == NULL) {
        return true;
    }

    RvAstSequence *selected = NULL;
    RvCheckError best = {0};
    for (RvAstSequence *branch = sequence; branch != NULL; branch = branch->alternative) {
        *checker->error = (RvCheckError){0};
        if (header_compatible(checker, branch->statement)) {
            if (selected != NULL) {
                return rv_check_fail(checker, RV_CHECK_NAME, branch->statement->span,
                                     "header remains ambiguous after name resolution");
            }
            selected = branch;
        } else {
            if (checker->error->kind == RV_CHECK_ALLOCATION || checker->error->kind == RV_CHECK_LIMIT) {
                return false;
            }
            if (best.kind == RV_CHECK_OK || checker->error->span.start > best.span.start) {
                best = *checker->error;
            }
        }
    }

    *checker->error = (RvCheckError){0};
    if (selected == NULL) {
        *checker->error = best;
        return false;
    }
    sequence->statement = selected->statement;
    sequence->next = selected->next;
    sequence->alternative = NULL;
    return true;
}

static bool nested_block(RvChecker *checker, RvAstStmt *block, bool *returns) {
    RvScope *scope = rv_check_scope(checker, checker->scope, block->span);
    return scope != NULL && rv_check_block(checker, block, scope, returns);
}

bool rv_check_block(RvChecker *checker, RvAstStmt *block, RvScope *scope, bool *returns) {
    *returns = false;
    if (checker->depth >= 256) {
        return rv_check_fail(checker, RV_CHECK_LIMIT, block->span, "semantic nesting limit exceeded");
    }

    RvScope *saved_scope = checker->scope;
    checker->scope = scope;
    ++checker->depth;
    bool success = true;

    for (RvAstSequence *sequence = block->as.statements; sequence != NULL; sequence = sequence->next) {
        bool statement_returns = false;
        if (!rv_check_step(checker, sequence->statement->span)
            || !select_sequence(checker, sequence)
            || !check_statement(checker, sequence->statement, &statement_returns)) {
            success = false;
            break;
        }
        *returns = *returns || statement_returns;
    }

    --checker->depth;
    checker->scope = saved_scope;
    return success;
}

static bool check_declaration(RvChecker *checker, RvAstBinding *binding) {
    RvType expected = rv_check_type(checker, binding->type);
    if (checker->error->kind != RV_CHECK_OK) {
        return false;
    }
    RvType type = rv_check_expression(checker, binding->initializer, expected);
    if (type.kind == RV_TYPE_INVALID) {
        return false;
    }
    if (type.kind == RV_TYPE_VOID) {
        return rv_check_fail(checker, RV_CHECK_TYPE, binding->initializer->span,
                             "void cannot initialize a value");
    }
    if (expected.kind != RV_TYPE_INVALID && !rv_type_equal(type, expected)) {
        return rv_check_fail(checker, RV_CHECK_TYPE, binding->initializer->span,
                             "initializer type does not match annotation");
    }
    if (!rv_check_fresh(checker, binding->initializer, type)) {
        return false;
    }

    RvSymbol *symbol = rv_check_symbol(checker, RV_SYMBOL_LOCAL, binding->name);
    if (symbol == NULL || !rv_check_define(checker, symbol)) {
        return false;
    }
    symbol->mutable = binding->mutable;
    symbol->type = type;
    binding->symbol = symbol;
    return true;
}

static bool check_assignment(RvChecker *checker, RvAstStmt *statement) {
    RvAstExpr *target = statement->as.assignment.target;
    RvType type = rv_check_expression(checker, target, (RvType){0});
    if (type.kind == RV_TYPE_INVALID) {
        return false;
    }

    RvAstExpr *base = target;
    if (target->kind == RV_AST_FIELD) {
        base = target->as.field.base;
    } else if (target->kind == RV_AST_INDEX) {
        base = target->as.index.base;
    }
    bool replacement = target->kind == RV_AST_NAME && type.kind == RV_TYPE_ARRAY;
    if (!rv_check_mutation(checker, base->symbol, target->span, replacement)) {
        return false;
    }

    RvAstExpr *value = statement->as.assignment.value;
    RvType actual = rv_check_expression(checker, value, type);
    if (actual.kind == RV_TYPE_INVALID) {
        return false;
    }
    if (!rv_type_equal(type, actual)) {
        return rv_check_fail(checker, RV_CHECK_TYPE, value->span, "assignment type mismatch");
    }
    return rv_check_fresh(checker, value, type);
}

static bool require_bool(RvChecker *checker, RvAstExpr *expression) {
    RvType type = rv_check_expression(checker, expression, (RvType){0});
    if (type.kind == RV_TYPE_INVALID) {
        return false;
    }
    if (type.kind != RV_TYPE_BOOL) {
        return rv_check_fail(checker, RV_CHECK_TYPE, expression->span, "condition must have type bool");
    }
    return true;
}

static bool check_choose(RvChecker *checker, RvAstStmt *statement, bool *returns) {
    RvType selector = rv_check_expression(checker, statement->as.choose.selector, (RvType){0});
    if (selector.kind == RV_TYPE_INVALID) {
        return false;
    }
    if (selector.kind != RV_TYPE_INT && selector.kind != RV_TYPE_TEXT) {
        return rv_check_fail(checker, RV_CHECK_TYPE, statement->as.choose.selector->span,
                             "choose selector must have type int or text");
    }

    bool all_return = true;
    for (RvAstCase *arm = statement->as.choose.cases; arm != NULL; arm = arm->next) {
        bool integer = arm->label->kind == RV_TOKEN_INTEGER;
        if ((selector.kind == RV_TYPE_INT) != integer) {
            return rv_check_fail(checker, RV_CHECK_TYPE, arm->label->span,
                                 "case literal type does not match selector");
        }
        if (integer) {
            RvAstExpr literal = {.kind = RV_AST_INTEGER, .span = arm->label->span};
            literal.as.atom = arm->label;
            if (!rv_check_integer(checker, &literal, false)) {
                return false;
            }
            arm->integer = literal.number.integer;
        }
        for (RvAstCase *prior = statement->as.choose.cases; prior != arm; prior = prior->next) {
            bool same = integer ? prior->integer == arm->integer
                : strcmp(prior->label->text, arm->label->text) == 0;
            if (same) {
                return rv_check_fail(checker, RV_CHECK_TYPE, arm->label->span, "duplicate case literal");
            }
        }
        bool branch_returns;
        if (!nested_block(checker, arm->body, &branch_returns)) {
            return false;
        }
        all_return = all_return && branch_returns;
    }

    bool otherwise_returns = false;
    if (statement->as.choose.otherwise != NULL
        && !nested_block(checker, statement->as.choose.otherwise, &otherwise_returns)) {
        return false;
    }
    *returns = all_return && otherwise_returns;
    return true;
}

static bool check_walk(RvChecker *checker, RvAstStmt *statement) {
    bool range = statement->kind == RV_AST_RANGE_WALK;
    const RvToken *name = range ? statement->as.range_walk.name : statement->as.each_walk.name;
    RvAstStmt *body = range ? statement->as.range_walk.body : statement->as.each_walk.body;
    RvType binding_type = {.kind = RV_TYPE_INT};
    RvSymbolKind binding_kind = RV_SYMBOL_LOOP;
    RvSymbol *target = NULL;

    if (range) {
        RvType begin = rv_check_expression(checker, statement->as.range_walk.begin, (RvType){0});
        RvType end = rv_check_expression(checker, statement->as.range_walk.end, (RvType){0});
        if (begin.kind == RV_TYPE_INVALID || end.kind == RV_TYPE_INVALID) {
            return false;
        }
        if (begin.kind != RV_TYPE_INT || end.kind != RV_TYPE_INT) {
            return rv_check_fail(checker, RV_CHECK_TYPE, statement->span, "range bounds must have type int");
        }
    } else {
        RvAstExpr *iterable = statement->as.each_walk.iterable;
        RvType type = rv_check_expression(checker, iterable, (RvType){0});
        if (type.kind == RV_TYPE_ARRAY) {
            binding_type.kind = type.element;
            RvAstExpr *value = rv_check_ungroup(iterable);
            if (value->kind == RV_AST_NAME) {
                target = value->symbol;
            }
        } else if (type.kind == RV_TYPE_RECORD) {
            binding_kind = RV_SYMBOL_PROJECTION;
            binding_type = (RvType){.kind = RV_TYPE_PROJECTION, .record = type.record};
        } else {
            return rv_check_fail(checker, RV_CHECK_TYPE, iterable->span,
                                 "walk requires an array or record");
        }
    }

    RvScope *scope = rv_check_scope(checker, checker->scope, body->span);
    if (scope == NULL) {
        return false;
    }
    RvSymbol *binding = rv_check_symbol(checker, binding_kind, name);
    if (binding == NULL) {
        return false;
    }
    binding->type = binding_type;
    RvScope *saved_scope = checker->scope;
    checker->scope = scope;
    bool defined = rv_check_define(checker, binding);
    checker->scope = saved_scope;
    if (!defined) {
        return false;
    }

    if (range) {
        statement->as.range_walk.binding = binding;
    } else {
        statement->as.each_walk.binding = binding;
    }
    RvActiveWalk active = {target, checker->walks};
    if (target != NULL) {
        checker->walks = &active;
    }
    bool ignored_returns;
    bool success = rv_check_block(checker, body, scope, &ignored_returns);
    checker->walks = active.next;
    return success;
}

static bool check_statement(RvChecker *checker, RvAstStmt *statement, bool *returns) {
    *returns = false;
    switch (statement->kind) {
        case RV_AST_BLOCK:
            return nested_block(checker, statement, returns);
        case RV_AST_DECLARATION:
            return check_declaration(checker, &statement->as.declaration);
        case RV_AST_ASSIGNMENT:
            return check_assignment(checker, statement);
        case RV_AST_EXPRESSION:
            return rv_check_expression(checker, statement->as.expression, (RvType){0}).kind
                != RV_TYPE_INVALID;
        case RV_AST_GIVE: {
            RvAstExpr *value = statement->as.expression;
            if (value == NULL) {
                if (checker->result.kind != RV_TYPE_VOID) {
                    return rv_check_fail(checker, RV_CHECK_RETURN, statement->span,
                                         "non-void function must return a value");
                }
            } else {
                if (checker->result.kind == RV_TYPE_VOID) {
                    return rv_check_fail(checker, RV_CHECK_RETURN, value->span,
                                         "void function cannot return a value");
                }
                RvType type = rv_check_expression(checker, value, checker->result);
                if (type.kind == RV_TYPE_INVALID) {
                    return false;
                }
                if (!rv_type_equal(type, checker->result)) {
                    return rv_check_fail(checker, RV_CHECK_RETURN, value->span, "return value type mismatch");
                }
            }
            *returns = true;
            return true;
        }
        case RV_AST_CHECK: {
            if (!require_bool(checker, statement->as.check.condition)) {
                return false;
            }
            bool body_returns;
            bool otherwise_returns = false;
            if (!nested_block(checker, statement->as.check.body, &body_returns)) {
                return false;
            }
            if (statement->as.check.otherwise != NULL
                && !nested_block(checker, statement->as.check.otherwise, &otherwise_returns)) {
                return false;
            }
            *returns = body_returns && otherwise_returns;
            return true;
        }
        case RV_AST_CHOOSE:
            return check_choose(checker, statement, returns);
        case RV_AST_RANGE_WALK:
        case RV_AST_EACH_WALK:
            return check_walk(checker, statement);
        case RV_AST_DURING: {
            bool ignored_returns;
            return require_bool(checker, statement->as.during.condition)
                && nested_block(checker, statement->as.during.body, &ignored_returns);
        }
    }
    return false;
}
