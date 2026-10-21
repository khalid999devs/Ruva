#include "test.h"

#include <string.h>

static bool following_blocks_keep_both_continuations(void) {
    TestProgram program;
    CHECK(test_parse_program(
        "launch() { check ready {} { show(1); } show(2); }", true, &program
    ));

    RvAstSequence *first = program.module.launch->as.function.body->as.statements;
    RvAstSequence *second = first->alternative;
    CHECK(second != NULL && second->alternative == NULL);
    CHECK(first->statement->as.check.condition->kind == RV_AST_NAME);
    CHECK(first->statement->as.check.body->as.statements == NULL);
    CHECK(first->next->statement->kind == RV_AST_BLOCK);
    CHECK(second->statement->as.check.condition->kind == RV_AST_RECORD);
    CHECK(second->statement->as.check.body->as.statements->statement->kind == RV_AST_EXPRESSION);
    CHECK(first->next->next == second->next);
    CHECK(second->next->statement->kind == RV_AST_EXPRESSION && second->next->next == NULL);

    test_program_destroy(&program);
    return true;
}

static bool all_header_forms_preserve_choices(void) {
    const char *inputs[] = {
        "launch() { during ready {} {} }",
        "launch() { choose value {} {} }",
        "launch() { walk field in Empty {} {} }",
        "launch() { walk index from 0 until bound {} {} }",
    };
    const RvAstStmtKind kinds[] = {
        RV_AST_DURING, RV_AST_CHOOSE, RV_AST_EACH_WALK, RV_AST_RANGE_WALK
    };

    for (size_t index = 0; index < sizeof(inputs) / sizeof(inputs[0]); ++index) {
        TestProgram program;
        CHECK(test_parse_program(inputs[index], true, &program));
        RvAstSequence *sequence = program.module.launch->as.function.body->as.statements;
        CHECK(sequence->statement->kind == kinds[index]);
        CHECK(sequence->alternative != NULL && sequence->alternative->alternative == NULL);
        CHECK(sequence->next->statement->kind == RV_AST_BLOCK);
        CHECK(sequence->alternative->next == NULL);
        test_program_destroy(&program);
    }

    return true;
}

static bool complete_context_prunes_impossible_choices(void) {
    TestProgram program;
    CHECK(test_parse_program("launch() { check ready {} {} otherwise {} }", true, &program));
    RvAstSequence *sequence = program.module.launch->as.function.body->as.statements;
    CHECK(sequence->alternative == NULL && sequence->next == NULL);
    CHECK(sequence->statement->as.check.condition->kind == RV_AST_RECORD);
    CHECK(sequence->statement->as.check.otherwise != NULL);
    test_program_destroy(&program);

    CHECK(test_parse_program(
        "launch() { check Empty {} == Other {} {} }", true, &program
    ));
    sequence = program.module.launch->as.function.body->as.statements;
    CHECK(sequence->alternative != NULL && sequence->alternative->alternative == NULL);
    CHECK(sequence->statement->as.check.condition->kind == RV_AST_BINARY);
    CHECK(sequence->statement->as.check.condition->as.binary.left->kind == RV_AST_RECORD);
    CHECK(sequence->statement->as.check.condition->as.binary.right->kind == RV_AST_NAME);
    CHECK(sequence->alternative->statement->as.check.condition->as.binary.right->kind == RV_AST_RECORD);
    test_program_destroy(&program);

    CHECK(test_parse_error(
        "launch() { check ready {} + 1; }", true, RV_PARSE_SYNTAX, NULL
    ));
    return true;
}

static bool parentheses_nonempty_records_and_shadowing(void) {
    TestProgram program;
    CHECK(test_parse_program(
        "object Empty {}"
        "launch() {"
        "check (Empty {}) {}"
        "check Pair { ready: true }.ready {}"
        "keep Empty = true;"
        "check Empty {} {}"
        "}", true, &program
    ));

    RvAstSequence *sequence = program.module.launch->as.function.body->as.statements;
    CHECK(sequence->alternative == NULL);
    CHECK(sequence->statement->as.check.condition->kind == RV_AST_GROUP);
    CHECK(sequence->statement->as.check.condition->as.group->kind == RV_AST_RECORD);
    sequence = sequence->next;
    CHECK(sequence->alternative == NULL);
    CHECK(sequence->statement->as.check.condition->kind == RV_AST_FIELD);
    sequence = sequence->next->next;
    CHECK(sequence->alternative != NULL);
    CHECK(sequence->statement->as.check.condition->kind == RV_AST_NAME);
    CHECK(sequence->alternative->statement->as.check.condition->kind == RV_AST_RECORD);

    test_program_destroy(&program);
    return true;
}

static bool many_independent_choices_share_suffixes(void) {
    char input[12000];
    const char prefix[] = "launch() {";
    const char statement[] = "check ready {} {} ";
    size_t length = sizeof(prefix) - 1;
    memcpy(input, prefix, length);

    for (size_t index = 0; index < 400; ++index) {
        memcpy(input + length, statement, sizeof(statement) - 1);
        length += sizeof(statement) - 1;
    }
    input[length++] = '}';
    input[length] = '\0';

    TestProgram program;
    CHECK(test_parse_program(input, true, &program));
    RvAstSequence *sequence = program.module.launch->as.function.body->as.statements;
    size_t count = 0;
    while (sequence != NULL) {
        CHECK(sequence->alternative != NULL);
        CHECK(sequence->next->statement->kind == RV_AST_BLOCK);
        CHECK(sequence->next->next == sequence->alternative->next);
        sequence = sequence->alternative->next;
        ++count;
    }
    CHECK(count == 400);

    FILE *dump = tmpfile();
    CHECK(dump != NULL);
    CHECK(!rv_ast_dump(dump, &program.module));
    CHECK(fclose(dump) == 0);

    test_program_destroy(&program);
    return true;
}

bool test_parser_ambiguity(void) {
    return following_blocks_keep_both_continuations()
        && all_header_forms_preserve_choices()
        && complete_context_prunes_impossible_choices()
        && parentheses_nonempty_records_and_shadowing()
        && many_independent_choices_share_suffixes();
}
