#include "test.h"

#include <string.h>

static bool binary_operator(const RvAstExpr *expression, RvTokenKind kind) {
    return expression != NULL && expression->kind == RV_AST_BINARY
        && expression->as.binary.operator->kind == kind;
}

static bool precedence_and_associativity(void) {
    TestProgram program;
    CHECK(test_parse_program(
        "launch() {"
        "keep result = 1 + 2 * 3 < 9 == true && false || true;"
        "keep difference = 10 - 3 - 2;"
        "keep remainder = 24 / 3 % 2;"
        "keep grouped = (1 + 2) * 3;"
        "keep unary = !--value;"
        "}", true, &program
    ));

    RvAstExpr *expression = test_initializer(&program, 0);
    CHECK(binary_operator(expression, RV_TOKEN_OR_OR));
    expression = expression->as.binary.left;
    CHECK(binary_operator(expression, RV_TOKEN_AND_AND));
    expression = expression->as.binary.left;
    CHECK(binary_operator(expression, RV_TOKEN_EQUAL_EQUAL));
    expression = expression->as.binary.left;
    CHECK(binary_operator(expression, RV_TOKEN_LESS));
    expression = expression->as.binary.left;
    CHECK(binary_operator(expression, RV_TOKEN_PLUS));
    CHECK(binary_operator(expression->as.binary.right, RV_TOKEN_STAR));

    expression = test_initializer(&program, 1);
    CHECK(binary_operator(expression, RV_TOKEN_MINUS));
    CHECK(binary_operator(expression->as.binary.left, RV_TOKEN_MINUS));
    CHECK(expression->as.binary.right->kind == RV_AST_INTEGER);
    CHECK(strcmp(expression->as.binary.right->as.atom->spelling, "2") == 0);

    expression = test_initializer(&program, 2);
    CHECK(binary_operator(expression, RV_TOKEN_PERCENT));
    CHECK(binary_operator(expression->as.binary.left, RV_TOKEN_SLASH));

    expression = test_initializer(&program, 3);
    CHECK(binary_operator(expression, RV_TOKEN_STAR));
    CHECK(expression->as.binary.left->kind == RV_AST_GROUP);
    CHECK(binary_operator(expression->as.binary.left->as.group, RV_TOKEN_PLUS));

    expression = test_initializer(&program, 4);
    CHECK(expression->kind == RV_AST_UNARY && expression->as.unary.operator->kind == RV_TOKEN_BANG);
    expression = expression->as.unary.operand;
    CHECK(expression->kind == RV_AST_UNARY && expression->as.unary.operator->kind == RV_TOKEN_MINUS);
    expression = expression->as.unary.operand;
    CHECK(expression->kind == RV_AST_UNARY && expression->as.unary.operator->kind == RV_TOKEN_MINUS);
    CHECK(expression->as.unary.operand->kind == RV_AST_NAME);

    test_program_destroy(&program);
    return true;
}

static bool postfix_arrays_and_ordered_records(void) {
    TestProgram program;
    CHECK(test_parse_program(
        "launch() {"
        "keep result = factory(1, ask(int))[2].key;"
        "keep values = [010, 08, -2147483648];"
        "keep pair = Pair { second: later(), first: earlier() };"
        "keep empty = Empty {};"
        "keep blank: text[] = [];"
        "}", true, &program
    ));

    RvAstExpr *expression = test_initializer(&program, 0);
    CHECK(expression->kind == RV_AST_FIELD);
    CHECK(strcmp(expression->as.field.name->spelling, "key") == 0);
    expression = expression->as.field.base;
    CHECK(expression->kind == RV_AST_INDEX);
    CHECK(expression->as.index.index->kind == RV_AST_INTEGER);
    expression = expression->as.index.base;
    CHECK(expression->kind == RV_AST_CALL);
    CHECK(strcmp(expression->as.call.callee->as.atom->spelling, "factory") == 0);
    CHECK(expression->as.call.arguments->next->value->kind == RV_AST_TYPED_ASK);
    CHECK(expression->as.call.arguments->next->next == NULL);

    RvAstExprList *values = test_initializer(&program, 1)->as.elements;
    CHECK(strcmp(values->value->as.atom->spelling, "010") == 0);
    CHECK(strcmp(values->next->value->as.atom->spelling, "08") == 0);
    CHECK(values->next->next->value->kind == RV_AST_UNARY);
    CHECK(strcmp(
        values->next->next->value->as.unary.operand->as.atom->spelling, "2147483648"
    ) == 0);

    expression = test_initializer(&program, 2);
    CHECK(expression->kind == RV_AST_RECORD);
    CHECK(strcmp(expression->as.record.name->spelling, "Pair") == 0);
    CHECK(strcmp(expression->as.record.fields->name->spelling, "second") == 0);
    CHECK(strcmp(expression->as.record.fields->next->name->spelling, "first") == 0);
    CHECK(expression->as.record.fields->next->next == NULL);
    CHECK(test_initializer(&program, 3)->as.record.fields == NULL);
    CHECK(test_initializer(&program, 4)->kind == RV_AST_ARRAY);
    CHECK(test_initializer(&program, 4)->as.elements == NULL);

    test_program_destroy(&program);
    return true;
}

static bool typed_ask_and_owned_literal_spans(void) {
    const char *input =
        "launch() {\n"
        "keep n = ask(int) + 1;\n"
        "keep r = toReal(ask(int)) + ask(real);\n"
        "keep b = ask(bool);\n"
        "keep t = ask(text);\n"
        "keep word = \"a\\n\";\n"
        "}";

    TestProgram program;
    CHECK(test_parse_program(input, true, &program));
    RvAstExpr *expression = test_initializer(&program, 0);
    CHECK(binary_operator(expression, RV_TOKEN_PLUS));
    CHECK(expression->as.binary.left->kind == RV_AST_TYPED_ASK);
    CHECK(expression->as.binary.left->as.ask_type.kind == RV_AST_TYPE_INT);

    expression = test_initializer(&program, 1);
    CHECK(expression->as.binary.left->kind == RV_AST_CALL);
    CHECK(expression->as.binary.left->as.call.arguments->value->kind == RV_AST_TYPED_ASK);
    CHECK(expression->as.binary.right->as.ask_type.kind == RV_AST_TYPE_REAL);
    CHECK(test_initializer(&program, 2)->as.ask_type.kind == RV_AST_TYPE_BOOL);
    CHECK(test_initializer(&program, 3)->as.ask_type.kind == RV_AST_TYPE_TEXT);

    expression = test_initializer(&program, 4);
    CHECK(expression->kind == RV_AST_TEXT);
    CHECK(strcmp(expression->as.atom->spelling, "\"a\\n\"") == 0);
    CHECK(expression->as.atom->text_length == 2 && strcmp(expression->as.atom->text, "a\n") == 0);
    CHECK(expression->span.start == (size_t)(strstr(input, "\"a\\n\"") - input));
    CHECK(expression->span.end == expression->span.start + 5);

    RvSourcePosition position;
    CHECK(rv_source_position(program.source, expression->span.start, &position));
    CHECK(position.line == 6 && position.column == 13);

    FILE *dump = tmpfile();
    CHECK(dump != NULL && rv_ast_dump(dump, &program.module));
    CHECK(fclose(dump) == 0);
    test_program_destroy(&program);
    return true;
}

static bool malformed_expressions(void) {
    const char *inputs[] = {
        "launch() { keep x = ; }",
        "launch() { keep x = (); }",
        "launch() { keep x = [1,]; }",
        "launch() { keep x = f(1,); }",
        "launch() { keep x = Pair { first: 1, }; }",
        "launch() { keep x = 1e3; }",
        "launch() { keep x = .5; }",
        "launch() { keep x = 0x10; }",
        "launch() { keep x = ask(); }",
        "launch() { keep x = ask(\"int\"); }",
        "launch() { keep x = ask(value); }",
        "launch() { keep x = ask(void); }",
        "launch() { keep x = ask(file); }",
        "launch() { keep x = ask(int[]); }",
        "launch() { keep x = ask(int, 1); }",
        "launch() { keep x = +1; }",
        "launch() { keep x = a = b; }",
    };

    for (size_t index = 0; index < sizeof(inputs) / sizeof(inputs[0]); ++index) {
        CHECK(test_parse_error(inputs[index], true, RV_PARSE_SYNTAX, NULL));
    }
    return true;
}

bool test_parser_expressions(void) {
    return precedence_and_associativity()
        && postfix_arrays_and_ordered_records()
        && typed_ask_and_owned_literal_spans()
        && malformed_expressions();
}
