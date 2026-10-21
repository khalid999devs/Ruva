#include "test.h"

#include <string.h>

static bool statements_and_control_flow(void) {
    TestProgram program;
    CHECK(test_parse_program(
        "launch() {"
        "vary values: int[] = [1, 2];"
        "keep box: Pair = Pair { amount: 2 };"
        "vary handle: file = fileOpen(\"out.txt\", \"w\");"
        "values[0] = pop(values);"
        "box.amount = 3;"
        "handle = fileOpen(\"next.txt\", \"w\");"
        "show(ask(text), values[0]);"
        "check ready { give; } otherwise { show(\"waiting\"); }"
        "choose code { when 1 => {} when \"two\" => {} otherwise => {} }"
        "walk index from start() until finish() { show(index); }"
        "walk item in values { show(item); }"
        "during ready { ready = false; }"
        "{ keep inner = true; }"
        "give;"
        "}", true, &program
    ));

    RvAstSequence *sequence = program.module.launch->as.function.body->as.statements;
    CHECK(sequence->statement->as.declaration.mutable);
    CHECK(sequence->statement->as.declaration.type.array);
    CHECK(sequence->statement->as.declaration.type.kind == RV_AST_TYPE_INT);
    sequence = sequence->next;
    CHECK(sequence->statement->as.declaration.type.kind == RV_AST_TYPE_NAMED);
    sequence = sequence->next;
    CHECK(sequence->statement->as.declaration.type.kind == RV_AST_TYPE_FILE);

    sequence = sequence->next;
    CHECK(sequence->statement->kind == RV_AST_ASSIGNMENT);
    CHECK(sequence->statement->as.assignment.target->kind == RV_AST_INDEX);
    CHECK(sequence->statement->as.assignment.value->kind == RV_AST_CALL);
    sequence = sequence->next;
    CHECK(sequence->statement->as.assignment.target->kind == RV_AST_FIELD);
    sequence = sequence->next;
    CHECK(sequence->statement->as.assignment.target->kind == RV_AST_NAME);
    sequence = sequence->next;
    CHECK(sequence->statement->kind == RV_AST_EXPRESSION);
    CHECK(sequence->statement->as.expression->as.call.arguments->value->kind == RV_AST_TYPED_ASK);

    sequence = sequence->next;
    CHECK(sequence->statement->kind == RV_AST_CHECK && sequence->alternative == NULL);
    CHECK(sequence->statement->as.check.condition->kind == RV_AST_NAME);
    CHECK(sequence->statement->as.check.otherwise != NULL);
    sequence = sequence->next;
    CHECK(sequence->statement->kind == RV_AST_CHOOSE);
    CHECK(sequence->statement->as.choose.cases->label->kind == RV_TOKEN_INTEGER);
    CHECK(sequence->statement->as.choose.cases->next->label->kind == RV_TOKEN_TEXT_LITERAL);
    CHECK(sequence->statement->as.choose.otherwise != NULL);

    sequence = sequence->next;
    CHECK(sequence->statement->kind == RV_AST_RANGE_WALK);
    CHECK(sequence->statement->as.range_walk.begin->kind == RV_AST_CALL);
    CHECK(sequence->statement->as.range_walk.end->kind == RV_AST_CALL);
    sequence = sequence->next;
    CHECK(sequence->statement->kind == RV_AST_EACH_WALK);
    CHECK(strcmp(sequence->statement->as.each_walk.name->spelling, "item") == 0);
    sequence = sequence->next;
    CHECK(sequence->statement->kind == RV_AST_DURING);
    sequence = sequence->next;
    CHECK(sequence->statement->kind == RV_AST_BLOCK);
    sequence = sequence->next;
    CHECK(sequence->statement->kind == RV_AST_GIVE && sequence->statement->as.expression == NULL);
    CHECK(sequence->next == NULL);

    test_program_destroy(&program);
    return true;
}

static bool semantic_questions_remain_unresolved(void) {
    TestProgram program;
    CHECK(test_parse_program(
        "keep large = 2147483648;"
        "object Pair { repeated: int; repeated: text; }"
        "craft noReturn() -> int {}"
        "launch() {"
        "keep fixed = 1;"
        "fixed = \"wrong type\";"
        "unknown();"
        "keep nested = [[1]];"
        "choose 1 { when 1 => {} when 1 => {} }"
        "give 1;"
        "}", true, &program
    ));
    CHECK(program.module.launch != NULL);
    test_program_destroy(&program);
    return true;
}

static bool malformed_statements(void) {
    const char *inputs[] = {
        "launch() { keep value; }",
        "launch() { keep value = 1 }",
        "launch() { keep value = 1;; }",
        "launch() { show(1) }",
        "launch() { give }",
        "launch() { ; }",
        "launch() { {}; }",
        "launch() { check true {}; }",
        "launch() { otherwise {} }",
        "launch() { check true {} otherwise check false {} }",
        "launch() { choose 1 { when -1 => {} } }",
        "launch() { choose 1 { when true => {} } }",
        "launch() { choose 1 { when 1.0 => {} } }",
        "launch() { choose 1 { when 1 {} } }",
        "launch() { choose 1 { otherwise => {} when 1 => {} } }",
        "launch() { choose 1 { otherwise => {} otherwise => {} } }",
        "launch() { walk item values {} }",
        "launch() { walk index from 0 {} }",
        "launch() { walk index from 0 until {} }",
        "launch() { during {} }",
        "launch() { 1 + 2; }",
        "launch() { value; }",
        "launch() { (value) = 1; }",
        "launch() { value.field.other = 1; }",
        "launch() { value[0][1] = 1; }",
        "launch() { make()[0] = 1; }",
        "launch() { value.method(); }",
        "launch() { a = b = 1; }",
        "launch() { craft nested() -> void {} }",
        "launch() { keep value: void = 1; }",
        "launch() { keep value: Pair[] = []; }",
        "launch() { keep value: int[][] = []; }",
        "launch() { keep value = [1); }",
    };

    for (size_t index = 0; index < sizeof(inputs) / sizeof(inputs[0]); ++index) {
        CHECK(test_parse_error(inputs[index], true, RV_PARSE_SYNTAX, NULL));
    }
    return true;
}

bool test_parser_statements(void) {
    return statements_and_control_flow()
        && semantic_questions_remain_unresolved()
        && malformed_statements();
}
