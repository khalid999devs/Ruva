#include "internal.h"

typedef struct RvParseCandidate {
    RvAstStmt *statement;
    size_t end;
    struct RvParseCandidate *next;
} RvParseCandidate;

typedef struct RvParseFrame {
    size_t start;
    RvParseCandidate *pending;
    RvAstSequence *head;
    RvAstSequence **tail;
    RvParseError best_error;
    struct RvParseFrame *previous;
} RvParseFrame;

enum {
    MEMO_UNSEEN,
    MEMO_VALID,
    MEMO_INVALID,
    MEMO_BUSY
};

static RvAstStmt *new_statement(RvParser *parser, RvAstStmtKind kind, size_t start) {
    RvAstStmt *statement = rv_parse_new(parser, sizeof(*statement));
    if (statement != NULL) {
        statement->kind = kind;
        statement->span = rv_parse_span(parser, start);
    }

    return statement;
}

static RvAstStmt *parse_local(RvParser *parser) {
    const RvToken *introducer = rv_parse_take(parser);
    const RvToken *name = rv_parse_expect(parser, RV_TOKEN_IDENTIFIER, "expected a binding name");
    if (name == NULL) {
        return NULL;
    }

    RvAstType type = {0};
    if (rv_parse_match(parser, RV_TOKEN_COLON)
        && !rv_parse_type(parser, RV_PARSE_LOCAL_TYPE, &type)) {
        return NULL;
    }

    if (rv_parse_expect(parser, RV_TOKEN_EQUAL, "expected '=' before initializer") == NULL) {
        return NULL;
    }

    RvAstExpr *initializer = rv_parse_expression(parser);
    if (initializer == NULL
        || rv_parse_expect(parser, RV_TOKEN_SEMICOLON, "expected ';' after declaration") == NULL) {
        return NULL;
    }

    RvAstStmt *statement = new_statement(parser, RV_AST_DECLARATION, introducer->span.start);
    if (statement != NULL) {
        statement->as.declaration = (RvAstBinding){
            .mutable = introducer->kind == RV_TOKEN_VARY,
            .name = name,
            .type = type,
            .initializer = initializer
        };
    }

    return statement;
}

static bool assignable_expression(const RvAstExpr *expression) {
    if (expression->kind == RV_AST_NAME) {
        return true;
    }

    if (expression->kind == RV_AST_FIELD) {
        return expression->as.field.base->kind == RV_AST_NAME;
    }

    if (expression->kind == RV_AST_INDEX) {
        return expression->as.index.base->kind == RV_AST_NAME;
    }

    return false;
}

static RvAstStmt *parse_expression_statement(RvParser *parser) {
    RvAstExpr *expression = rv_parse_expression(parser);
    if (expression == NULL) {
        return NULL;
    }

    RvAstExpr *value = NULL;
    RvAstStmtKind kind = RV_AST_EXPRESSION;
    if (rv_parse_match(parser, RV_TOKEN_EQUAL)) {
        if (!assignable_expression(expression)) {
            rv_parse_fail(parser, RV_PARSE_SYNTAX, expression->span,
                          "assignment requires a name, name.field or name[index]");
            return NULL;
        }

        kind = RV_AST_ASSIGNMENT;
        value = rv_parse_expression(parser);
        if (value == NULL) {
            return NULL;
        }
    } else {
        bool call = expression->kind == RV_AST_TYPED_ASK
            || (expression->kind == RV_AST_CALL && expression->as.call.callee->kind == RV_AST_NAME);
        if (!call) {
            rv_parse_fail(parser, RV_PARSE_SYNTAX, expression->span,
                          "expected an assignment or named call statement");
            return NULL;
        }
    }

    if (rv_parse_expect(parser, RV_TOKEN_SEMICOLON, "expected ';' after statement") == NULL) {
        return NULL;
    }

    RvAstStmt *statement = new_statement(parser, kind, expression->span.start);
    if (statement != NULL) {
        if (kind == RV_AST_ASSIGNMENT) {
            statement->as.assignment.target = expression;
            statement->as.assignment.value = value;
        } else {
            statement->as.expression = expression;
        }
    }

    return statement;
}

static RvAstStmt *parse_simple_statement(RvParser *parser) {
    RvTokenKind kind = rv_parse_peek(parser)->kind;
    if (kind == RV_TOKEN_KEEP || kind == RV_TOKEN_VARY) {
        return parse_local(parser);
    }

    if (kind == RV_TOKEN_LEFT_BRACE) {
        return rv_parse_block(parser);
    }

    if (kind == RV_TOKEN_GIVE) {
        size_t start = rv_parse_take(parser)->span.start;
        RvAstExpr *value = NULL;
        if (rv_parse_peek(parser)->kind != RV_TOKEN_SEMICOLON) {
            value = rv_parse_expression(parser);
            if (value == NULL) {
                return NULL;
            }
        }

        if (rv_parse_expect(parser, RV_TOKEN_SEMICOLON, "expected ';' after give") == NULL) {
            return NULL;
        }

        RvAstStmt *statement = new_statement(parser, RV_AST_GIVE, start);
        if (statement != NULL) {
            statement->as.expression = value;
        }
        return statement;
    }

    return parse_expression_statement(parser);
}

static RvParseCandidate *candidate(RvParser *parser, RvAstStmt *statement) {
    RvParseCandidate *result = rv_parse_scratch(parser, sizeof(*result));
    if (result != NULL) {
        result->statement = statement;
        result->end = parser->cursor;
    }

    return result;
}

static size_t find_until(RvParser *parser) {
    size_t index = parser->cursor;
    while (index < parser->limit) {
        RvTokenKind kind = parser->tokens[index].kind;
        if (kind == RV_TOKEN_UNTIL) {
            return index;
        }

        if (kind == RV_TOKEN_LEFT_PAREN || kind == RV_TOKEN_LEFT_BRACKET
            || kind == RV_TOKEN_LEFT_BRACE) {
            index = parser->closing[index] + 1;
        } else if (kind == RV_TOKEN_SEMICOLON || kind == RV_TOKEN_RIGHT_BRACE) {
            break;
        } else {
            ++index;
        }
    }

    rv_parse_fail(parser, RV_PARSE_SYNTAX, rv_parse_peek(parser)->span,
                  "expected until after range start");
    return parser->limit;
}

static bool parse_choose_body(RvParser *parser, RvAstStmt *statement) {
    if (rv_parse_expect(parser, RV_TOKEN_LEFT_BRACE, "expected '{' after choose selector") == NULL) {
        return false;
    }

    RvAstCase **tail = &statement->as.choose.cases;
    while (rv_parse_match(parser, RV_TOKEN_WHEN)) {
        const RvToken *label = rv_parse_peek(parser);
        if (label->kind != RV_TOKEN_INTEGER && label->kind != RV_TOKEN_TEXT_LITERAL) {
            rv_parse_fail(parser, RV_PARSE_SYNTAX, label->span,
                          "when requires an unsigned integer or text literal");
            return false;
        }

        rv_parse_take(parser);
        if (rv_parse_expect(parser, RV_TOKEN_FAT_ARROW, "expected '=>' after when literal") == NULL) {
            return false;
        }

        RvAstStmt *body = rv_parse_block(parser);
        if (body == NULL) {
            return false;
        }

        RvAstCase *arm = rv_parse_new(parser, sizeof(*arm));
        if (arm == NULL) {
            return false;
        }

        arm->label = label;
        arm->body = body;
        *tail = arm;
        tail = &arm->next;
    }

    if (rv_parse_match(parser, RV_TOKEN_OTHERWISE)) {
        if (rv_parse_expect(parser, RV_TOKEN_FAT_ARROW, "expected '=>' after otherwise") == NULL) {
            return false;
        }

        statement->as.choose.otherwise = rv_parse_block(parser);
        if (statement->as.choose.otherwise == NULL) {
            return false;
        }
    }

    return rv_parse_expect(
        parser, RV_TOKEN_RIGHT_BRACE, "expected '}' after choose arms; otherwise must be last"
    ) != NULL;
}

static bool header_continuation(RvTokenKind kind) {
    return kind == RV_TOKEN_IDENTIFIER || kind == RV_TOKEN_INTEGER
        || kind == RV_TOKEN_REAL_LITERAL || kind == RV_TOKEN_TEXT_LITERAL
        || kind == RV_TOKEN_TRUE || kind == RV_TOKEN_FALSE
        || kind >= RV_TOKEN_EQUAL;
}

static RvParseCandidate *parse_header(RvParser *parser, RvParseError *best_error) {
    const RvToken *introducer = rv_parse_take(parser);
    size_t block_end = parser->limit;
    const RvToken *binding = NULL;
    RvAstExpr *begin = NULL;
    RvAstStmtKind kind;

    switch (introducer->kind) {
        case RV_TOKEN_CHECK:
            kind = RV_AST_CHECK;
            break;
        case RV_TOKEN_CHOOSE:
            kind = RV_AST_CHOOSE;
            break;
        case RV_TOKEN_DURING:
            kind = RV_AST_DURING;
            break;
        default:
            kind = RV_AST_EACH_WALK;
            binding = rv_parse_expect(parser, RV_TOKEN_IDENTIFIER, "expected a walk binding");
            if (binding == NULL) {
                return NULL;
            }

            if (rv_parse_match(parser, RV_TOKEN_FROM)) {
                kind = RV_AST_RANGE_WALK;
                size_t until = find_until(parser);
                if (parser->error.kind != RV_PARSE_OK) {
                    return NULL;
                }

                parser->limit = until;
                begin = rv_parse_expression(parser);
                bool complete = begin != NULL && parser->cursor == until;
                parser->limit = block_end;
                if (!complete) {
                    if (begin != NULL) {
                        rv_parse_fail(parser, RV_PARSE_SYNTAX, rv_parse_peek(parser)->span,
                                      "expected until after range start");
                    }
                    return NULL;
                }
                rv_parse_take(parser);
            } else if (!rv_parse_match(parser, RV_TOKEN_IN)) {
                rv_parse_fail(parser, RV_PARSE_SYNTAX, rv_parse_peek(parser)->span,
                              "expected from or in after walk binding");
                return NULL;
            }
            break;
    }

    size_t expression_start = parser->cursor;
    RvParseCandidate *head = NULL;
    RvParseCandidate **tail = &head;
    size_t probe = expression_start;

    while (probe < block_end && !parser->fatal) {
        RvTokenKind token_kind = parser->tokens[probe].kind;
        if (token_kind == RV_TOKEN_LEFT_PAREN || token_kind == RV_TOKEN_LEFT_BRACKET) {
            probe = parser->closing[probe] + 1;
            continue;
        }

        if (token_kind == RV_TOKEN_SEMICOLON || token_kind == RV_TOKEN_RIGHT_BRACE
            || !header_continuation(token_kind)) {
            break;
        }

        if (token_kind != RV_TOKEN_LEFT_BRACE) {
            ++probe;
            continue;
        }

        parser->cursor = expression_start;
        parser->limit = probe;
        parser->error = (RvParseError){0};
        RvAstExpr *expression = rv_parse_expression(parser);
        bool complete = expression != NULL && parser->cursor == probe;
        parser->limit = block_end;

        if (!complete) {
            if (expression != NULL) {
                rv_parse_fail(parser, RV_PARSE_SYNTAX, rv_parse_peek(parser)->span,
                              "unexpected token in statement header");
            }
            rv_parse_keep_error(best_error, parser->error);
            break;
        }

        parser->cursor = probe;
        RvAstStmt *statement = new_statement(parser, kind, introducer->span.start);
        if (statement == NULL) {
            return NULL;
        }

        bool success;
        if (kind == RV_AST_CHOOSE) {
            statement->as.choose.selector = expression;
            success = parse_choose_body(parser, statement);
        } else {
            RvAstStmt *body = rv_parse_block(parser);
            success = body != NULL;
            if (success) {
                if (kind == RV_AST_CHECK) {
                    statement->as.check.condition = expression;
                    statement->as.check.body = body;
                    if (rv_parse_match(parser, RV_TOKEN_OTHERWISE)) {
                        statement->as.check.otherwise = rv_parse_block(parser);
                        success = statement->as.check.otherwise != NULL;
                    }
                } else if (kind == RV_AST_DURING) {
                    statement->as.during.condition = expression;
                    statement->as.during.body = body;
                } else if (kind == RV_AST_EACH_WALK) {
                    statement->as.each_walk.name = binding;
                    statement->as.each_walk.iterable = expression;
                    statement->as.each_walk.body = body;
                } else {
                    statement->as.range_walk.name = binding;
                    statement->as.range_walk.begin = begin;
                    statement->as.range_walk.end = expression;
                    statement->as.range_walk.body = body;
                }
            }
        }

        if (parser->fatal) {
            return NULL;
        }

        if (success) {
            statement->span = rv_parse_span(parser, introducer->span.start);
            RvParseCandidate *item = candidate(parser, statement);
            if (item == NULL) {
                return NULL;
            }
            *tail = item;
            tail = &item->next;
        } else {
            rv_parse_keep_error(best_error, parser->error);
        }

        probe = parser->closing[probe] + 1;
    }

    parser->limit = block_end;
    if (!parser->fatal) {
        parser->error = (RvParseError){0};
        if (head == NULL) {
            if (best_error->kind != RV_PARSE_OK) {
                parser->error = *best_error;
            } else {
                parser->cursor = probe;
                rv_parse_fail(parser, RV_PARSE_SYNTAX, rv_parse_peek(parser)->span,
                              "expected a statement body");
            }
        }
    }

    return head;
}

static RvParseFrame *new_frame(RvParser *parser, size_t start, size_t end,
                                RvParseFrame *previous) {
    parser->cursor = start;
    parser->limit = end;
    parser->error = (RvParseError){0};

    RvParseFrame *frame = rv_parse_scratch(parser, sizeof(*frame));
    if (frame == NULL) {
        return NULL;
    }

    frame->start = start;
    frame->previous = previous;
    frame->tail = &frame->head;
    parser->memo[start].status = MEMO_BUSY;
    RvTokenKind kind = rv_parse_peek(parser)->kind;

    if (kind == RV_TOKEN_CHECK || kind == RV_TOKEN_CHOOSE
        || kind == RV_TOKEN_DURING || kind == RV_TOKEN_WALK) {
        frame->pending = parse_header(parser, &frame->best_error);
    } else {
        RvAstStmt *statement = parse_simple_statement(parser);
        if (statement != NULL) {
            frame->pending = candidate(parser, statement);
        }
    }

    rv_parse_keep_error(&frame->best_error, parser->error);
    return parser->fatal ? NULL : frame;
}

static bool parse_sequence(RvParser *parser, size_t start, size_t end,
                            RvAstSequence **output) {
    if (start == end) {
        *output = NULL;
        return true;
    }

    if (parser->memo[start].status == MEMO_UNSEEN) {
        RvParseFrame *frame = new_frame(parser, start, end, NULL);
        while (frame != NULL && !parser->fatal) {
            RvParseCandidate *current = frame->pending;
            if (current == NULL) {
                RvParseMemo *memo = &parser->memo[frame->start];
                memo->status = frame->head == NULL ? MEMO_INVALID : MEMO_VALID;
                memo->sequence = frame->head;
                memo->error = frame->best_error;
                frame = frame->previous;
                continue;
            }

            if (current->end <= frame->start || current->end > end) {
                rv_parse_fail(parser, RV_PARSE_LIMIT, current->statement->span,
                              "parser continuation did not advance");
                return false;
            }

            RvAstSequence *suffix = NULL;
            if (current->end < end) {
                RvParseMemo *memo = &parser->memo[current->end];
                if (memo->status == MEMO_UNSEEN) {
                    frame = new_frame(parser, current->end, end, frame);
                    continue;
                }

                if (memo->status == MEMO_BUSY) {
                    rv_parse_fail(parser, RV_PARSE_LIMIT, current->statement->span,
                                  "cyclic parser continuation");
                    return false;
                }

                if (memo->status != MEMO_VALID) {
                    rv_parse_keep_error(&frame->best_error, memo->error);
                    frame->pending = current->next;
                    continue;
                }
                suffix = memo->sequence;
            }

            RvAstSequence *sequence = rv_parse_new(parser, sizeof(*sequence));
            if (sequence == NULL) {
                return false;
            }

            sequence->statement = current->statement;
            sequence->next = suffix;
            *frame->tail = sequence;
            frame->tail = &sequence->alternative;
            frame->pending = current->next;
        }
    }

    if (parser->fatal) {
        return false;
    }

    RvParseMemo *memo = &parser->memo[start];
    if (memo->status != MEMO_VALID) {
        parser->error = memo->error;
        return false;
    }

    parser->error = (RvParseError){0};
    *output = memo->sequence;
    return true;
}

RvAstStmt *rv_parse_block(RvParser *parser) {
    if (!rv_parse_enter(parser)) {
        return NULL;
    }

    size_t opening = parser->cursor;
    size_t saved_limit = parser->limit;
    const RvToken *token = rv_parse_expect(parser, RV_TOKEN_LEFT_BRACE, "expected '{' for block");
    RvAstStmt *block = NULL;

    if (token != NULL) {
        size_t closing = parser->closing[opening];
        RvAstSequence *sequence = NULL;
        if (closing < saved_limit && parse_sequence(parser, opening + 1, closing, &sequence)) {
            parser->limit = saved_limit;
            parser->cursor = closing + 1;
            block = new_statement(parser, RV_AST_BLOCK, token->span.start);
            if (block != NULL) {
                block->as.statements = sequence;
            }
        } else if (parser->error.kind == RV_PARSE_OK) {
            rv_parse_fail(parser, RV_PARSE_SYNTAX, token->span, "block crosses its enclosing boundary");
        }
    }

    parser->limit = saved_limit;
    --parser->depth;
    return block;
}
