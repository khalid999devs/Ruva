#include "ast.h"

typedef struct {
    FILE *output;
    size_t depth;
    size_t remaining;
    bool valid;
} AstWriter;

static void print_expression(AstWriter *writer, const RvAstExpr *expression);
static void print_statement(AstWriter *writer, const RvAstStmt *statement);
static void print_sequence(AstWriter *writer, const RvAstSequence *sequence);

static bool open_node(AstWriter *writer, const char *name) {
    if (!writer->valid || writer->remaining == 0 || writer->depth >= 256) {
        writer->valid = false;
        return false;
    }

    --writer->remaining;
    for (size_t index = 0; index < writer->depth; ++index) {
        fputs("  ", writer->output);
    }
    fprintf(writer->output, "(%s", name);
    ++writer->depth;
    return true;
}

static void close_node(AstWriter *writer) {
    if (!writer->valid) {
        return;
    }

    --writer->depth;
    for (size_t index = 0; index < writer->depth; ++index) {
        fputs("  ", writer->output);
    }
    fputs(")\n", writer->output);
}

static void print_text(AstWriter *writer, const char *bytes, size_t length) {
    fputc('"', writer->output);
    for (size_t index = 0; index < length; ++index) {
        unsigned char byte = (unsigned char)bytes[index];
        switch (byte) {
            case '\n':
                fputs("\\n", writer->output);
                break;
            case '\r':
                fputs("\\r", writer->output);
                break;
            case '\t':
                fputs("\\t", writer->output);
                break;
            case '"':
                fputs("\\\"", writer->output);
                break;
            case '\\':
                fputs("\\\\", writer->output);
                break;
            default:
                if (byte < 0x20 || byte > 0x7e) {
                    fprintf(writer->output, "\\x%02x", byte);
                } else {
                    fputc(byte, writer->output);
                }
                break;
        }
    }
    fputc('"', writer->output);
}

static void print_type(AstWriter *writer, RvAstType type) {
    static const char *const names[] = {
        "infer", "int", "real", "bool", "text", "void", "file", "named"
    };

    const char *name = type.kind == RV_AST_TYPE_NAMED ? type.name->spelling : names[type.kind];
    fprintf(writer->output, " %s%s", name, type.array ? "[]" : "");
}

static void print_arguments(AstWriter *writer, const RvAstExprList *arguments) {
    for (const RvAstExprList *item = arguments; item != NULL && writer->valid; item = item->next) {
        print_expression(writer, item->value);
    }
}

static void print_expression(AstWriter *writer, const RvAstExpr *expression) {
    static const char *const names[] = {
        "integer", "real", "text", "bool", "name", "group", "unary",
        "binary", "call", "index", "field", "array", "record", "ask"
    };

    if (!open_node(writer, names[expression->kind])) {
        return;
    }

    switch (expression->kind) {
        case RV_AST_INTEGER:
        case RV_AST_REAL:
        case RV_AST_BOOL:
        case RV_AST_NAME:
            fprintf(writer->output, " %s)\n", expression->as.atom->spelling);
            --writer->depth;
            return;
        case RV_AST_TEXT:
            fputc(' ', writer->output);
            print_text(writer, expression->as.atom->text, expression->as.atom->text_length);
            fputs(")\n", writer->output);
            --writer->depth;
            return;
        case RV_AST_TYPED_ASK:
            print_type(writer, expression->as.ask_type);
            fputs(")\n", writer->output);
            --writer->depth;
            return;
        case RV_AST_UNARY:
            fprintf(writer->output, " %s\n", expression->as.unary.operator->spelling);
            print_expression(writer, expression->as.unary.operand);
            break;
        case RV_AST_BINARY:
            fprintf(writer->output, " %s\n", expression->as.binary.operator->spelling);
            print_expression(writer, expression->as.binary.left);
            print_expression(writer, expression->as.binary.right);
            break;
        case RV_AST_GROUP:
            fputc('\n', writer->output);
            print_expression(writer, expression->as.group);
            break;
        case RV_AST_CALL:
            fputc('\n', writer->output);
            print_expression(writer, expression->as.call.callee);
            print_arguments(writer, expression->as.call.arguments);
            break;
        case RV_AST_INDEX:
            fputc('\n', writer->output);
            print_expression(writer, expression->as.index.base);
            print_expression(writer, expression->as.index.index);
            break;
        case RV_AST_FIELD:
            fprintf(writer->output, " %s\n", expression->as.field.name->spelling);
            print_expression(writer, expression->as.field.base);
            break;
        case RV_AST_ARRAY:
            fputc('\n', writer->output);
            print_arguments(writer, expression->as.elements);
            break;
        case RV_AST_RECORD:
            fprintf(writer->output, " %s\n", expression->as.record.name->spelling);
            for (const RvAstFieldInit *field = expression->as.record.fields;
                 field != NULL && writer->valid; field = field->next) {
                if (!open_node(writer, "field")) {
                    break;
                }
                fprintf(writer->output, " %s\n", field->name->spelling);
                print_expression(writer, field->value);
                close_node(writer);
            }
            break;
    }

    close_node(writer);
}

static void print_sequence(AstWriter *writer, const RvAstSequence *sequence) {
    while (sequence != NULL && writer->valid) {
        if (sequence->alternative == NULL) {
            print_statement(writer, sequence->statement);
            sequence = sequence->next;
            continue;
        }

        if (!open_node(writer, "choice")) {
            return;
        }
        fputc('\n', writer->output);

        for (const RvAstSequence *branch = sequence;
             branch != NULL && writer->valid; branch = branch->alternative) {
            if (!open_node(writer, "sequence")) {
                break;
            }
            fputc('\n', writer->output);
            print_statement(writer, branch->statement);
            print_sequence(writer, branch->next);
            close_node(writer);
        }

        close_node(writer);
        return;
    }
}

static void print_binding(AstWriter *writer, const RvAstBinding *binding) {
    fprintf(writer->output, " %s", binding->name->spelling);
    print_type(writer, binding->type);
    fputc('\n', writer->output);
    print_expression(writer, binding->initializer);
}

static void print_statement(AstWriter *writer, const RvAstStmt *statement) {
    static const char *const names[] = {
        "block", "declaration", "assign", "expression", "give",
        "check", "choose", "range-walk", "each-walk", "during"
    };
    const char *name = names[statement->kind];
    if (statement->kind == RV_AST_DECLARATION) {
        name = statement->as.declaration.mutable ? "vary" : "keep";
    }

    if (!open_node(writer, name)) {
        return;
    }

    switch (statement->kind) {
        case RV_AST_BLOCK:
            fputc('\n', writer->output);
            print_sequence(writer, statement->as.statements);
            break;
        case RV_AST_DECLARATION:
            print_binding(writer, &statement->as.declaration);
            break;
        case RV_AST_ASSIGNMENT:
            fputc('\n', writer->output);
            print_expression(writer, statement->as.assignment.target);
            print_expression(writer, statement->as.assignment.value);
            break;
        case RV_AST_EXPRESSION:
        case RV_AST_GIVE:
            fputc('\n', writer->output);
            if (statement->as.expression != NULL) {
                print_expression(writer, statement->as.expression);
            }
            break;
        case RV_AST_CHECK:
            fputc('\n', writer->output);
            print_expression(writer, statement->as.check.condition);
            print_statement(writer, statement->as.check.body);
            if (statement->as.check.otherwise != NULL) {
                print_statement(writer, statement->as.check.otherwise);
            }
            break;
        case RV_AST_CHOOSE:
            fputc('\n', writer->output);
            print_expression(writer, statement->as.choose.selector);
            for (const RvAstCase *arm = statement->as.choose.cases;
                 arm != NULL && writer->valid; arm = arm->next) {
                if (!open_node(writer, "when")) {
                    break;
                }
                fputc(' ', writer->output);
                print_text(writer, arm->label->spelling, arm->label->length);
                fputc('\n', writer->output);
                print_statement(writer, arm->body);
                close_node(writer);
            }
            if (statement->as.choose.otherwise != NULL && open_node(writer, "otherwise")) {
                fputc('\n', writer->output);
                print_statement(writer, statement->as.choose.otherwise);
                close_node(writer);
            }
            break;
        case RV_AST_RANGE_WALK:
            fprintf(writer->output, " %s\n", statement->as.range_walk.name->spelling);
            print_expression(writer, statement->as.range_walk.begin);
            print_expression(writer, statement->as.range_walk.end);
            print_statement(writer, statement->as.range_walk.body);
            break;
        case RV_AST_EACH_WALK:
            fprintf(writer->output, " %s\n", statement->as.each_walk.name->spelling);
            print_expression(writer, statement->as.each_walk.iterable);
            print_statement(writer, statement->as.each_walk.body);
            break;
        case RV_AST_DURING:
            fputc('\n', writer->output);
            print_expression(writer, statement->as.during.condition);
            print_statement(writer, statement->as.during.body);
            break;
    }

    close_node(writer);
}

static void print_declaration(AstWriter *writer, const RvAstDecl *declaration) {
    static const char *const names[] = {"constant", "craft", "object", "launch"};
    if (!open_node(writer, names[declaration->kind])) {
        return;
    }

    if (declaration->kind != RV_AST_LAUNCH) {
        fprintf(writer->output, " %s%s", declaration->name->spelling,
                declaration->shared ? " share" : "");
    }
    fputc('\n', writer->output);

    if (declaration->kind == RV_AST_CONSTANT) {
        if (open_node(writer, "keep")) {
            print_binding(writer, &declaration->as.constant);
            close_node(writer);
        }
    } else if (declaration->kind == RV_AST_OBJECT) {
        for (const RvAstMember *field = declaration->as.fields;
             field != NULL && writer->valid; field = field->next) {
            if (!open_node(writer, "field")) {
                break;
            }
            fprintf(writer->output, " %s", field->name->spelling);
            print_type(writer, field->type);
            fputs(")\n", writer->output);
            --writer->depth;
        }
    } else {
        for (const RvAstMember *parameter = declaration->as.function.parameters;
             parameter != NULL && writer->valid; parameter = parameter->next) {
            if (!open_node(writer, "parameter")) {
                break;
            }
            fprintf(writer->output, " %s", parameter->name->spelling);
            print_type(writer, parameter->type);
            fputs(")\n", writer->output);
            --writer->depth;
        }
        if (open_node(writer, "result")) {
            print_type(writer, declaration->as.function.result);
            fputs(")\n", writer->output);
            --writer->depth;
        }
        print_statement(writer, declaration->as.function.body);
    }

    close_node(writer);
}

bool rv_ast_dump(FILE *output, const RvAstModule *module) {
    if (output == NULL || module == NULL || module->source == NULL) {
        return false;
    }

    AstWriter writer = {output, 0, 100000, true};
    if (!open_node(&writer, module->entry ? "entry" : "module")) {
        return false;
    }
    fputc('\n', output);

    for (const RvAstImport *import = module->imports;
         import != NULL && writer.valid; import = import->next) {
        if (!open_node(&writer, "bring")) {
            break;
        }
        fputc(' ', output);
        print_text(&writer, import->path->text, import->path->text_length);
        for (const RvAstNameList *name = import->names; name != NULL; name = name->next) {
            fprintf(output, " %s", name->name->spelling);
        }
        fputs(")\n", output);
        --writer.depth;
    }

    for (const RvAstDecl *declaration = module->declarations;
         declaration != NULL && writer.valid; declaration = declaration->next) {
        print_declaration(&writer, declaration);
    }
    if (module->launch != NULL && writer.valid) {
        print_declaration(&writer, module->launch);
    }

    close_node(&writer);
    return writer.valid && !ferror(output);
}
