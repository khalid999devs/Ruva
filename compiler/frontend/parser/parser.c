#include "internal.h"

#include <string.h>

const RvToken *rv_parse_peek(const RvParser *parser) {
    return &parser->tokens[parser->cursor];
}

const RvToken *rv_parse_take(RvParser *parser) {
    const RvToken *token = rv_parse_peek(parser);
    if (parser->cursor < parser->limit) {
        ++parser->cursor;
    }

    return token;
}

bool rv_parse_match(RvParser *parser, RvTokenKind kind) {
    if (parser->cursor < parser->limit && rv_parse_peek(parser)->kind == kind) {
        ++parser->cursor;
        return true;
    }

    return false;
}

const RvToken *rv_parse_expect(RvParser *parser, RvTokenKind kind, const char *message) {
    const RvToken *token = rv_parse_peek(parser);
    if (!rv_parse_match(parser, kind)) {
        rv_parse_fail(parser, RV_PARSE_SYNTAX, token->span, message);
        return NULL;
    }

    return token;
}

void rv_parse_fail(RvParser *parser, RvParseErrorKind kind, RvSpan span, const char *message) {
    if (parser->error.kind == RV_PARSE_OK || kind == RV_PARSE_ALLOCATION || kind == RV_PARSE_LIMIT) {
        parser->error = (RvParseError){kind, span, message};
    }

    if (kind == RV_PARSE_ALLOCATION || kind == RV_PARSE_LIMIT) {
        parser->fatal = true;
    }
}

void rv_parse_keep_error(RvParseError *best, RvParseError candidate) {
    if (candidate.kind != RV_PARSE_OK
        && (best->kind == RV_PARSE_OK || candidate.span.start > best->span.start)) {
        *best = candidate;
    }
}

static void *allocate_node(RvParser *parser, RvArena *arena, size_t size) {
    if (parser->fatal) {
        return NULL;
    }

    RvSpan span = {parser->module->source, 0, 0};
    if (parser->tokens != NULL) {
        span = rv_parse_peek(parser)->span;
    }

    if (++parser->work > parser->work_limit) {
        rv_parse_fail(parser, RV_PARSE_LIMIT, span, "parser work limit exceeded");
        return NULL;
    }

    void *node = rv_arena_allocate(arena, size);
    if (node == NULL) {
        rv_parse_fail(parser, RV_PARSE_ALLOCATION, span, "out of memory while parsing source");
        return NULL;
    }

    memset(node, 0, size);
    return node;
}

void *rv_parse_new(RvParser *parser, size_t size) {
    return allocate_node(parser, &parser->module->storage, size);
}

void *rv_parse_scratch(RvParser *parser, size_t size) {
    return allocate_node(parser, &parser->scratch, size);
}

bool rv_parse_enter(RvParser *parser) {
    if (parser->depth >= RV_PARSE_MAX_DEPTH) {
        rv_parse_fail(parser, RV_PARSE_LIMIT, rv_parse_peek(parser)->span,
                      "parser nesting limit exceeded");
        return false;
    }

    ++parser->depth;
    return true;
}

RvSpan rv_parse_span(const RvParser *parser, size_t start) {
    size_t end = start;
    if (parser->cursor != 0) {
        end = parser->tokens[parser->cursor - 1].span.end;
    }

    return (RvSpan){parser->module->source, start, end};
}

bool rv_parse_type(RvParser *parser, RvParseTypeContext context, RvAstType *type) {
    const RvToken *name = rv_parse_peek(parser);
    RvAstTypeKind kind;

    switch (name->kind) {
        case RV_TOKEN_INT:
            kind = RV_AST_TYPE_INT;
            break;
        case RV_TOKEN_REAL:
            kind = RV_AST_TYPE_REAL;
            break;
        case RV_TOKEN_BOOL:
            kind = RV_AST_TYPE_BOOL;
            break;
        case RV_TOKEN_TEXT:
            kind = RV_AST_TYPE_TEXT;
            break;
        case RV_TOKEN_FILE:
            kind = RV_AST_TYPE_FILE;
            break;
        case RV_TOKEN_VOID:
            kind = RV_AST_TYPE_VOID;
            break;
        case RV_TOKEN_IDENTIFIER:
            kind = RV_AST_TYPE_NAMED;
            break;
        default:
            rv_parse_fail(parser, RV_PARSE_SYNTAX, name->span, "expected a type");
            return false;
    }

    bool primitive = kind >= RV_AST_TYPE_INT && kind <= RV_AST_TYPE_TEXT;
    bool permitted = primitive
        || (kind == RV_AST_TYPE_NAMED && context != RV_PARSE_PRIMITIVE_TYPE)
        || (kind == RV_AST_TYPE_FILE && context == RV_PARSE_LOCAL_TYPE)
        || (kind == RV_AST_TYPE_VOID && context == RV_PARSE_RESULT_TYPE);

    if (!permitted) {
        rv_parse_fail(parser, RV_PARSE_SYNTAX, name->span, "type is not allowed here");
        return false;
    }

    rv_parse_take(parser);
    bool array = false;
    if (rv_parse_peek(parser)->kind == RV_TOKEN_LEFT_BRACKET) {
        if (!primitive || context != RV_PARSE_LOCAL_TYPE) {
            rv_parse_fail(parser, RV_PARSE_SYNTAX, rv_parse_peek(parser)->span,
                          "array type is not allowed here");
            return false;
        }

        rv_parse_take(parser);
        if (rv_parse_expect(parser, RV_TOKEN_RIGHT_BRACKET, "expected ']' in array type") == NULL) {
            return false;
        }
        array = true;
    }

    *type = (RvAstType){kind, array, name, rv_parse_span(parser, name->span.start)};
    return true;
}

static bool validate_tokens(RvParser *parser, const RvTokenStream *stream) {
    if (stream == NULL || stream->tokens.data == NULL || stream->tokens.length == 0
        || stream->tokens.element_size != sizeof(RvToken)
        || stream->tokens.length > stream->tokens.capacity) {
        rv_parse_fail(parser, RV_PARSE_INVALID_ARGUMENT, (RvSpan){0},
                      "a complete token stream is required");
        return false;
    }

    const RvToken *tokens = (const RvToken *)stream->tokens.data;
    const RvSource *source = tokens[0].span.source;
    if (source == NULL) {
        rv_parse_fail(parser, RV_PARSE_INVALID_ARGUMENT, (RvSpan){0},
                      "tokens require a source");
        return false;
    }

    size_t previous_end = 0;
    for (size_t index = 0; index < stream->tokens.length; ++index) {
        const RvToken *token = &tokens[index];
        bool last = index + 1 == stream->tokens.length;

        if (token->kind <= RV_TOKEN_INVALID || token->kind >= RV_TOKEN_COUNT
            || (token->kind == RV_TOKEN_EOF) != last || token->spelling == NULL
            || token->span.source != source || token->span.start < previous_end
            || token->span.start > token->span.end || token->span.end > rv_source_length(source)
            || token->length != token->span.end - token->span.start
            || (token->kind == RV_TOKEN_TEXT_LITERAL && token->text == NULL)
            || (last && (token->length != 0 || token->span.start != rv_source_length(source)))) {
            rv_parse_fail(parser, RV_PARSE_INVALID_ARGUMENT, (RvSpan){0},
                          "invalid token stream");
            return false;
        }

        previous_end = token->span.end;
    }

    parser->module->source = source;
    parser->count = stream->tokens.length;
    parser->limit = parser->count - 1;
    return true;
}

static bool copy_tokens(RvParser *parser, const RvTokenStream *stream) {
    size_t size;
    if (!rv_size_multiply(parser->count, sizeof(RvToken), &size)) {
        rv_parse_fail(parser, RV_PARSE_LIMIT, (RvSpan){0}, "token stream is too large");
        return false;
    }

    parser->tokens = rv_parse_new(parser, size);
    if (parser->tokens == NULL) {
        return false;
    }

    for (size_t index = 0; index < parser->count; ++index) {
        const RvToken *original = rv_token_stream_at(stream, index);
        RvToken *copy = &parser->tokens[index];
        *copy = *original;
        copy->spelling = rv_arena_copy(
            &parser->module->storage, original->spelling, original->length
        );

        if (original->text != NULL) {
            copy->text = rv_arena_copy(
                &parser->module->storage, original->text, original->text_length
            );
        }

        if (copy->spelling == NULL || (original->text != NULL && copy->text == NULL)) {
            rv_parse_fail(parser, RV_PARSE_ALLOCATION, original->span,
                          "out of memory while parsing source");
            return false;
        }
    }

    return true;
}

static bool index_delimiters(RvParser *parser) {
    size_t size;
    size_t memo_size;
    if (!rv_size_multiply(parser->count, sizeof(size_t), &size)
        || !rv_size_multiply(parser->count, sizeof(RvParseMemo), &memo_size)) {
        rv_parse_fail(parser, RV_PARSE_LIMIT, rv_parse_peek(parser)->span,
                      "token stream is too large");
        return false;
    }

    parser->closing = rv_parse_scratch(parser, size);
    size_t *stack = rv_parse_scratch(parser, size);
    parser->memo = rv_parse_scratch(parser, memo_size);
    if (parser->closing == NULL || stack == NULL || parser->memo == NULL) {
        return false;
    }

    size_t depth = 0;
    for (size_t index = 0; index < parser->limit; ++index) {
        RvTokenKind kind = parser->tokens[index].kind;
        if (kind == RV_TOKEN_LEFT_PAREN || kind == RV_TOKEN_LEFT_BRACKET
            || kind == RV_TOKEN_LEFT_BRACE) {
            stack[depth++] = index;
            continue;
        }

        RvTokenKind opening;
        switch (kind) {
            case RV_TOKEN_RIGHT_PAREN:
                opening = RV_TOKEN_LEFT_PAREN;
                break;
            case RV_TOKEN_RIGHT_BRACKET:
                opening = RV_TOKEN_LEFT_BRACKET;
                break;
            case RV_TOKEN_RIGHT_BRACE:
                opening = RV_TOKEN_LEFT_BRACE;
                break;
            default:
                continue;
        }

        if (depth == 0 || parser->tokens[stack[depth - 1]].kind != opening) {
            rv_parse_fail(parser, RV_PARSE_SYNTAX, parser->tokens[index].span,
                          "mismatched closing delimiter");
            return false;
        }

        parser->closing[stack[--depth]] = index;
    }

    if (depth != 0) {
        rv_parse_fail(parser, RV_PARSE_SYNTAX, parser->tokens[stack[depth - 1]].span,
                      "unclosed delimiter");
        return false;
    }

    return true;
}

bool rv_parse_module(const RvTokenStream *tokens, bool entry, RvAllocator allocator,
                      RvAstModule *output, RvParseError *error) {
    if (error != NULL) {
        *error = (RvParseError){0};
    }

    if (output == NULL) {
        if (error != NULL) {
            *error = (RvParseError){
                .kind = RV_PARSE_INVALID_ARGUMENT,
                .message = "AST output is required"
            };
        }
        return false;
    }

    *output = (RvAstModule){.entry = entry};
    rv_arena_init(&output->storage, allocator, 4096);

    RvParser parser = {.module = output};
    rv_arena_init(&parser.scratch, allocator, 4096);
    bool success = validate_tokens(&parser, tokens);

    if (success) {
        success = rv_size_multiply(parser.count, 64, &parser.work_limit)
            && rv_size_add(parser.work_limit, 1024, &parser.work_limit);
        if (!success) {
            rv_parse_fail(&parser, RV_PARSE_LIMIT, (RvSpan){0}, "token stream is too large");
        }
    }

    if (success) {
        success = copy_tokens(&parser, tokens)
            && index_delimiters(&parser)
            && rv_parse_declarations(&parser);
    }

    rv_arena_destroy(&parser.scratch);
    if (!success) {
        rv_ast_module_destroy(output);
    } else {
        output->span = (RvSpan){output->source, 0, rv_source_length(output->source)};
    }

    if (error != NULL) {
        *error = parser.error;
    }

    return success;
}
