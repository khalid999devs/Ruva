#include "internal.h"

#include <string.h>

static RvAstExpr *parse_unary(RvParser *parser);

static size_t greater_depth(size_t left, size_t right) {
    return left > right ? left : right;
}

RvAstExpr *rv_parse_expr_new(RvParser *parser, RvAstExprKind kind,
                             RvSpan span, size_t child_depth) {
    if (child_depth >= RV_PARSE_MAX_DEPTH) {
        rv_parse_fail(parser, RV_PARSE_LIMIT, span, "expression nesting limit exceeded");
        return NULL;
    }

    RvAstExpr *expression = rv_parse_new(parser, sizeof(*expression));
    if (expression != NULL) {
        expression->kind = kind;
        expression->span = span;
        expression->depth = child_depth + 1;
    }

    return expression;
}

static RvAstExpr *parse_atom(RvParser *parser) {
    const RvToken *token = rv_parse_peek(parser);
    RvAstExprKind kind;

    switch (token->kind) {
        case RV_TOKEN_INTEGER:
            kind = RV_AST_INTEGER;
            break;
        case RV_TOKEN_REAL_LITERAL:
            kind = RV_AST_REAL;
            break;
        case RV_TOKEN_TEXT_LITERAL:
            kind = RV_AST_TEXT;
            break;
        case RV_TOKEN_TRUE:
        case RV_TOKEN_FALSE:
            kind = RV_AST_BOOL;
            break;
        case RV_TOKEN_IDENTIFIER:
            kind = RV_AST_NAME;
            break;
        default:
            rv_parse_fail(parser, RV_PARSE_SYNTAX, token->span, "expected an expression");
            return NULL;
    }

    if (parser->cursor == parser->limit) {
        rv_parse_fail(parser, RV_PARSE_SYNTAX, token->span, "expected an expression");
        return NULL;
    }

    rv_parse_take(parser);
    RvAstExpr *expression = rv_parse_expr_new(parser, kind, token->span, 0);
    if (expression != NULL) {
        expression->as.atom = token;
    }

    return expression;
}

static bool parse_arguments(RvParser *parser, RvTokenKind closing,
                             RvAstExprList **output, size_t *depth) {
    RvAstExprList **tail = output;
    if (rv_parse_match(parser, closing)) {
        return true;
    }

    do {
        RvAstExpr *value = rv_parse_expression(parser);
        if (value == NULL) {
            return false;
        }

        RvAstExprList *item = rv_parse_new(parser, sizeof(*item));
        if (item == NULL) {
            return false;
        }

        item->value = value;
        *tail = item;
        tail = &item->next;
        *depth = greater_depth(*depth, value->depth);
    } while (rv_parse_match(parser, RV_TOKEN_COMMA));

    const char *message = closing == RV_TOKEN_RIGHT_PAREN ? "expected ')'" : "expected ']'";
    return rv_parse_expect(parser, closing, message) != NULL;
}

static RvAstExpr *parse_record(RvParser *parser, const RvToken *name) {
    rv_parse_take(parser);
    RvAstFieldInit *fields = NULL;
    RvAstFieldInit **tail = &fields;
    size_t depth = 0;

    if (!rv_parse_match(parser, RV_TOKEN_RIGHT_BRACE)) {
        do {
            const RvToken *field = rv_parse_expect(
                parser, RV_TOKEN_IDENTIFIER, "expected a record field name"
            );
            if (field == NULL
                || rv_parse_expect(parser, RV_TOKEN_COLON, "expected ':' after field name") == NULL) {
                return NULL;
            }

            RvAstExpr *value = rv_parse_expression(parser);
            if (value == NULL) {
                return NULL;
            }

            RvAstFieldInit *item = rv_parse_new(parser, sizeof(*item));
            if (item == NULL) {
                return NULL;
            }

            item->name = field;
            item->value = value;
            *tail = item;
            tail = &item->next;
            depth = greater_depth(depth, value->depth);
        } while (rv_parse_match(parser, RV_TOKEN_COMMA));

        if (rv_parse_expect(parser, RV_TOKEN_RIGHT_BRACE, "expected '}' after record fields") == NULL) {
            return NULL;
        }
    }

    RvAstExpr *expression = rv_parse_expr_new(
        parser, RV_AST_RECORD, rv_parse_span(parser, name->span.start), depth
    );
    if (expression != NULL) {
        expression->as.record.name = name;
        expression->as.record.fields = fields;
    }

    return expression;
}

static RvAstExpr *parse_ask(RvParser *parser, const RvToken *name) {
    rv_parse_take(parser);
    RvAstType type;
    if (!rv_parse_type(parser, RV_PARSE_PRIMITIVE_TYPE, &type)
        || rv_parse_expect(parser, RV_TOKEN_RIGHT_PAREN, "expected ')' after ask type") == NULL) {
        return NULL;
    }

    RvAstExpr *expression = rv_parse_expr_new(
        parser, RV_AST_TYPED_ASK, rv_parse_span(parser, name->span.start), 0
    );
    if (expression != NULL) {
        expression->as.ask_type = type;
    }

    return expression;
}

static RvAstExpr *parse_primary(RvParser *parser) {
    const RvToken *token = rv_parse_peek(parser);
    if (rv_parse_match(parser, RV_TOKEN_LEFT_PAREN)) {
        RvAstExpr *inner = rv_parse_expression(parser);
        if (inner == NULL
            || rv_parse_expect(parser, RV_TOKEN_RIGHT_PAREN, "expected ')'") == NULL) {
            return NULL;
        }

        RvAstExpr *group = rv_parse_expr_new(
            parser, RV_AST_GROUP, rv_parse_span(parser, token->span.start), inner->depth
        );
        if (group != NULL) {
            group->as.group = inner;
        }
        return group;
    }

    if (rv_parse_match(parser, RV_TOKEN_LEFT_BRACKET)) {
        RvAstExprList *elements = NULL;
        size_t depth = 0;
        if (!parse_arguments(parser, RV_TOKEN_RIGHT_BRACKET, &elements, &depth)) {
            return NULL;
        }

        RvAstExpr *array = rv_parse_expr_new(
            parser, RV_AST_ARRAY, rv_parse_span(parser, token->span.start), depth
        );
        if (array != NULL) {
            array->as.elements = elements;
        }
        return array;
    }

    RvAstExpr *atom = parse_atom(parser);
    if (atom == NULL || atom->kind != RV_AST_NAME || parser->cursor == parser->limit) {
        return atom;
    }

    if (rv_parse_peek(parser)->kind == RV_TOKEN_LEFT_PAREN
        && strcmp(token->spelling, "ask") == 0) {
        return parse_ask(parser, token);
    }

    if (rv_parse_peek(parser)->kind == RV_TOKEN_LEFT_BRACE) {
        return parse_record(parser, token);
    }

    return atom;
}

static RvAstExpr *parse_postfix(RvParser *parser) {
    RvAstExpr *expression = parse_primary(parser);
    while (expression != NULL && parser->cursor < parser->limit) {
        size_t start = expression->span.start;
        RvAstExpr *next;

        if (rv_parse_match(parser, RV_TOKEN_LEFT_PAREN)) {
            RvAstExprList *arguments = NULL;
            size_t depth = expression->depth;
            if (!parse_arguments(parser, RV_TOKEN_RIGHT_PAREN, &arguments, &depth)) {
                return NULL;
            }

            next = rv_parse_expr_new(parser, RV_AST_CALL, rv_parse_span(parser, start), depth);
            if (next != NULL) {
                next->as.call.callee = expression;
                next->as.call.arguments = arguments;
            }
        } else if (rv_parse_match(parser, RV_TOKEN_LEFT_BRACKET)) {
            RvAstExpr *index = rv_parse_expression(parser);
            if (index == NULL
                || rv_parse_expect(parser, RV_TOKEN_RIGHT_BRACKET, "expected ']'") == NULL) {
                return NULL;
            }

            size_t depth = greater_depth(expression->depth, index->depth);
            next = rv_parse_expr_new(parser, RV_AST_INDEX, rv_parse_span(parser, start), depth);
            if (next != NULL) {
                next->as.index.base = expression;
                next->as.index.index = index;
            }
        } else if (rv_parse_match(parser, RV_TOKEN_DOT)) {
            const RvToken *field = rv_parse_expect(
                parser, RV_TOKEN_IDENTIFIER, "expected a field name after '.'"
            );
            if (field == NULL) {
                return NULL;
            }

            next = rv_parse_expr_new(
                parser, RV_AST_FIELD, rv_parse_span(parser, start), expression->depth
            );
            if (next != NULL) {
                next->as.field.base = expression;
                next->as.field.name = field;
            }
        } else {
            break;
        }

        expression = next;
    }

    return expression;
}

static RvAstExpr *parse_unary(RvParser *parser) {
    if (!rv_parse_enter(parser)) {
        return NULL;
    }

    const RvToken *operator = rv_parse_peek(parser);
    RvAstExpr *expression;
    if (rv_parse_match(parser, RV_TOKEN_BANG) || rv_parse_match(parser, RV_TOKEN_MINUS)) {
        RvAstExpr *operand = parse_unary(parser);
        expression = NULL;
        if (operand != NULL) {
            expression = rv_parse_expr_new(
                parser, RV_AST_UNARY, rv_parse_span(parser, operator->span.start), operand->depth
            );
            if (expression != NULL) {
                expression->as.unary.operator = operator;
                expression->as.unary.operand = operand;
            }
        }
    } else {
        expression = parse_postfix(parser);
    }

    --parser->depth;
    return expression;
}

static unsigned int operator_precedence(RvTokenKind kind) {
    switch (kind) {
        case RV_TOKEN_OR_OR:
            return 1;
        case RV_TOKEN_AND_AND:
            return 2;
        case RV_TOKEN_EQUAL_EQUAL:
        case RV_TOKEN_BANG_EQUAL:
            return 3;
        case RV_TOKEN_LESS:
        case RV_TOKEN_LESS_EQUAL:
        case RV_TOKEN_GREATER:
        case RV_TOKEN_GREATER_EQUAL:
            return 4;
        case RV_TOKEN_PLUS:
        case RV_TOKEN_MINUS:
            return 5;
        case RV_TOKEN_STAR:
        case RV_TOKEN_SLASH:
        case RV_TOKEN_PERCENT:
            return 6;
        default:
            return 0;
    }
}

static RvAstExpr *parse_binary(RvParser *parser, unsigned int minimum) {
    RvAstExpr *left = parse_unary(parser);
    while (left != NULL && parser->cursor < parser->limit) {
        const RvToken *operator = rv_parse_peek(parser);
        unsigned int precedence = operator_precedence(operator->kind);
        if (precedence < minimum) {
            break;
        }

        rv_parse_take(parser);
        RvAstExpr *right = parse_binary(parser, precedence + 1);
        if (right == NULL) {
            return NULL;
        }

        size_t depth = greater_depth(left->depth, right->depth);
        RvSpan span = {parser->module->source, left->span.start, right->span.end};
        RvAstExpr *binary = rv_parse_expr_new(parser, RV_AST_BINARY, span, depth);
        if (binary == NULL) {
            return NULL;
        }

        binary->as.binary.operator = operator;
        binary->as.binary.left = left;
        binary->as.binary.right = right;
        left = binary;
    }

    return left;
}

RvAstExpr *rv_parse_expression(RvParser *parser) {
    if (!rv_parse_enter(parser)) {
        return NULL;
    }

    RvAstExpr *expression = parse_binary(parser, 1);
    --parser->depth;
    return expression;
}

RvAstExpr *rv_parse_constant(RvParser *parser) {
    const RvToken *minus = NULL;
    if (rv_parse_peek(parser)->kind == RV_TOKEN_MINUS) {
        minus = rv_parse_take(parser);
    }

    RvTokenKind kind = rv_parse_peek(parser)->kind;
    bool numeric = kind == RV_TOKEN_INTEGER || kind == RV_TOKEN_REAL_LITERAL;
    if ((!numeric && minus != NULL)
        || (!numeric && kind != RV_TOKEN_TEXT_LITERAL
            && kind != RV_TOKEN_TRUE && kind != RV_TOKEN_FALSE)) {
        rv_parse_fail(parser, RV_PARSE_SYNTAX, rv_parse_peek(parser)->span,
                      "expected a constant literal");
        return NULL;
    }

    RvAstExpr *literal = parse_atom(parser);
    if (literal == NULL || minus == NULL) {
        return literal;
    }

    RvAstExpr *negative = rv_parse_expr_new(
        parser, RV_AST_UNARY, rv_parse_span(parser, minus->span.start), literal->depth
    );
    if (negative != NULL) {
        negative->as.unary.operator = minus;
        negative->as.unary.operand = literal;
    }

    return negative;
}
