#include "internal.h"

static RvAstImport *parse_import(RvParser *parser) {
    size_t start = rv_parse_take(parser)->span.start;
    if (rv_parse_expect(parser, RV_TOKEN_LEFT_BRACE, "expected '{' after bring") == NULL) {
        return NULL;
    }

    RvAstNameList *names = NULL;
    RvAstNameList **tail = &names;
    do {
        const RvToken *name = rv_parse_expect(
            parser, RV_TOKEN_IDENTIFIER, "expected an imported name"
        );
        if (name == NULL) {
            return NULL;
        }

        RvAstNameList *item = rv_parse_new(parser, sizeof(*item));
        if (item == NULL) {
            return NULL;
        }

        item->name = name;
        *tail = item;
        tail = &item->next;
    } while (rv_parse_match(parser, RV_TOKEN_COMMA));

    if (rv_parse_expect(parser, RV_TOKEN_RIGHT_BRACE, "expected '}' after imported names") == NULL
        || rv_parse_expect(parser, RV_TOKEN_FROM, "expected from after imported names") == NULL) {
        return NULL;
    }

    const RvToken *path = rv_parse_expect(
        parser, RV_TOKEN_TEXT_LITERAL, "expected an import path text literal"
    );
    if (path == NULL
        || rv_parse_expect(parser, RV_TOKEN_SEMICOLON, "expected ';' after import") == NULL) {
        return NULL;
    }

    RvAstImport *import = rv_parse_new(parser, sizeof(*import));
    if (import != NULL) {
        import->span = rv_parse_span(parser, start);
        import->names = names;
        import->path = path;
    }

    return import;
}

static RvAstMember *parse_member(RvParser *parser, RvParseTypeContext context) {
    const RvToken *name = rv_parse_expect(parser, RV_TOKEN_IDENTIFIER, "expected a name");
    if (name == NULL
        || rv_parse_expect(parser, RV_TOKEN_COLON, "expected ':' after name") == NULL) {
        return NULL;
    }

    RvAstType type;
    if (!rv_parse_type(parser, context, &type)) {
        return NULL;
    }

    RvAstMember *member = rv_parse_new(parser, sizeof(*member));
    if (member != NULL) {
        member->name = name;
        member->type = type;
    }

    return member;
}

static bool parse_object(RvParser *parser, RvAstDecl *declaration) {
    if (rv_parse_expect(parser, RV_TOKEN_LEFT_BRACE, "expected '{' after object name") == NULL) {
        return false;
    }

    RvAstMember **tail = &declaration->as.fields;
    while (rv_parse_peek(parser)->kind != RV_TOKEN_RIGHT_BRACE
           && parser->cursor < parser->limit) {
        RvAstMember *field = parse_member(parser, RV_PARSE_PRIMITIVE_TYPE);
        if (field == NULL
            || rv_parse_expect(parser, RV_TOKEN_SEMICOLON, "expected ';' after field") == NULL) {
            return false;
        }

        *tail = field;
        tail = &field->next;
    }

    return rv_parse_expect(parser, RV_TOKEN_RIGHT_BRACE, "expected '}' after object fields") != NULL;
}

static bool parse_function(RvParser *parser, RvAstDecl *declaration) {
    if (rv_parse_expect(parser, RV_TOKEN_LEFT_PAREN, "expected '(' after function name") == NULL) {
        return false;
    }

    RvAstMember **tail = &declaration->as.function.parameters;
    if (!rv_parse_match(parser, RV_TOKEN_RIGHT_PAREN)) {
        do {
            RvAstMember *parameter = parse_member(parser, RV_PARSE_PARAMETER_TYPE);
            if (parameter == NULL) {
                return false;
            }

            *tail = parameter;
            tail = &parameter->next;
        } while (rv_parse_match(parser, RV_TOKEN_COMMA));

        if (rv_parse_expect(parser, RV_TOKEN_RIGHT_PAREN, "expected ')' after parameters") == NULL) {
            return false;
        }
    }

    if (rv_parse_expect(parser, RV_TOKEN_ARROW, "expected '->' before return type") == NULL
        || !rv_parse_type(parser, RV_PARSE_RESULT_TYPE, &declaration->as.function.result)) {
        return false;
    }

    declaration->as.function.body = rv_parse_block(parser);
    return declaration->as.function.body != NULL;
}

static bool parse_constant(RvParser *parser, RvAstDecl *declaration) {
    RvAstBinding *binding = &declaration->as.constant;
    binding->name = declaration->name;
    if (rv_parse_match(parser, RV_TOKEN_COLON)
        && !rv_parse_type(parser, RV_PARSE_PRIMITIVE_TYPE, &binding->type)) {
        return false;
    }

    if (rv_parse_expect(parser, RV_TOKEN_EQUAL, "expected '=' in constant declaration") == NULL) {
        return false;
    }

    binding->initializer = rv_parse_constant(parser);
    return binding->initializer != NULL
        && rv_parse_expect(parser, RV_TOKEN_SEMICOLON, "expected ';' after constant") != NULL;
}

static RvAstDecl *parse_top_declaration(RvParser *parser) {
    size_t start = rv_parse_peek(parser)->span.start;
    bool shared = rv_parse_match(parser, RV_TOKEN_SHARE);
    const RvToken *introducer = rv_parse_take(parser);
    RvAstDeclKind kind;

    switch (introducer->kind) {
        case RV_TOKEN_KEEP:
            kind = RV_AST_CONSTANT;
            break;
        case RV_TOKEN_CRAFT:
            kind = RV_AST_FUNCTION;
            break;
        case RV_TOKEN_OBJECT:
            kind = RV_AST_OBJECT;
            break;
        default:
            rv_parse_fail(parser, RV_PARSE_SYNTAX, introducer->span,
                          "expected a top-level keep, craft or object declaration");
            return NULL;
    }

    const RvToken *name = rv_parse_expect(
        parser, RV_TOKEN_IDENTIFIER, "expected a declaration name"
    );
    if (name == NULL) {
        return NULL;
    }

    RvAstDecl *declaration = rv_parse_new(parser, sizeof(*declaration));
    if (declaration == NULL) {
        return NULL;
    }

    declaration->kind = kind;
    declaration->shared = shared;
    declaration->name = name;

    bool success;
    if (kind == RV_AST_OBJECT) {
        success = parse_object(parser, declaration);
    } else if (kind == RV_AST_FUNCTION) {
        success = parse_function(parser, declaration);
    } else {
        success = parse_constant(parser, declaration);
    }

    if (!success) {
        return NULL;
    }

    declaration->span = rv_parse_span(parser, start);
    return declaration;
}

static RvAstDecl *parse_launch(RvParser *parser) {
    const RvToken *name = rv_parse_take(parser);
    if (rv_parse_expect(parser, RV_TOKEN_LEFT_PAREN, "expected '(' after launch") == NULL
        || rv_parse_expect(parser, RV_TOKEN_RIGHT_PAREN, "launch takes no parameters") == NULL) {
        return NULL;
    }

    RvAstStmt *body = rv_parse_block(parser);
    if (body == NULL) {
        return NULL;
    }

    RvAstDecl *launch = rv_parse_new(parser, sizeof(*launch));
    if (launch != NULL) {
        launch->kind = RV_AST_LAUNCH;
        launch->name = name;
        launch->span = rv_parse_span(parser, name->span.start);
        launch->as.function.result.kind = RV_AST_TYPE_VOID;
        launch->as.function.body = body;
    }

    return launch;
}

bool rv_parse_declarations(RvParser *parser) {
    RvAstImport **imports = &parser->module->imports;
    while (rv_parse_peek(parser)->kind == RV_TOKEN_BRING) {
        RvAstImport *import = parse_import(parser);
        if (import == NULL) {
            return false;
        }

        *imports = import;
        imports = &import->next;
    }

    RvAstDecl **declarations = &parser->module->declarations;
    while (parser->cursor < parser->limit) {
        if (rv_parse_peek(parser)->kind == RV_TOKEN_LAUNCH) {
            if (!parser->module->entry) {
                rv_parse_fail(parser, RV_PARSE_SYNTAX, rv_parse_peek(parser)->span,
                              "imported modules cannot define launch");
                return false;
            }

            parser->module->launch = parse_launch(parser);
            if (parser->module->launch == NULL) {
                return false;
            }

            if (parser->cursor != parser->limit) {
                rv_parse_fail(parser, RV_PARSE_SYNTAX, rv_parse_peek(parser)->span,
                              "launch must be the last declaration");
                return false;
            }
            return true;
        }

        if (rv_parse_peek(parser)->kind == RV_TOKEN_BRING) {
            rv_parse_fail(parser, RV_PARSE_SYNTAX, rv_parse_peek(parser)->span,
                          "imports must precede declarations");
            return false;
        }

        RvAstDecl *declaration = parse_top_declaration(parser);
        if (declaration == NULL) {
            return false;
        }

        *declarations = declaration;
        declarations = &declaration->next;
    }

    if (parser->module->entry) {
        rv_parse_fail(parser, RV_PARSE_SYNTAX, rv_parse_peek(parser)->span,
                      "entry file requires launch()");
        return false;
    }

    return true;
}
