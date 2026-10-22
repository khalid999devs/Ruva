#include "test.h"

#include <string.h>

static bool operators_and_calls(void) {
    const SemanticCase cases[] = {
        {"launch() { show(1 + 2 * 3, 8 / 2, 8 % 3, -2, 1 < 2, 2 <= 2, 3 > 1, 3 >= 3); }", RV_CHECK_OK, NULL},
        {"launch() { show(1.0 + 2.0, -1.0, 3.0 / 2.0, \"a\" + \"b\", \"a\" == \"b\", true != false, !false && true || false); }", RV_CHECK_OK, NULL},
        {"launch() { show(1 + 2.0); }", RV_CHECK_TYPE, "same type"},
        {"launch() { show(2.0 % 1.0); }", RV_CHECK_TYPE, "operator"},
        {"launch() { show(\"a\" < \"b\"); }", RV_CHECK_TYPE, "operator"},
        {"launch() { show(true + false); }", RV_CHECK_TYPE, "operator"},
        {"launch() { show(-true); }", RV_CHECK_TYPE, "unary"},
        {"launch() { show(!1); }", RV_CHECK_TYPE, "unary"},
        {"launch() { show([1] == [1]); }", RV_CHECK_TYPE, "operator"},
        {"launch() { keep value: real = 1; }", RV_CHECK_TYPE, "annotation"},
        {"launch() { vary value = 1; value = 1.0; }", RV_CHECK_TYPE, "assignment"},
        {"launch() { keep value: Missing = 1; }", RV_CHECK_TYPE, "record type"},
        {"launch() { check 1 {} }", RV_CHECK_TYPE, "bool"},
        {"launch() { during 1 {} }", RV_CHECK_TYPE, "bool"},
        {"launch() { walk i from 0 until 1.0 {} }", RV_CHECK_TYPE, "range"},
        {"launch() { walk i in 1 {} }", RV_CHECK_TYPE, "walk"},
        {"launch() { choose true {} }", RV_CHECK_TYPE, "selector"},
        {"launch() { choose 1 { when \"1\" => {} } }", RV_CHECK_TYPE, "case"},
        {"launch() { choose 1 { when 01 => {} when 1 => {} } }", RV_CHECK_TYPE, "duplicate case"},
        {"launch() { choose \"a\" { when \"a\" => {} when \"a\" => {} } }", RV_CHECK_TYPE, "duplicate case"},
        {"craft sum(n: int) -> int { check n == 0 { give 0; } otherwise { give n + sum(n - 1); } } launch() { show(sum(3), later()); } craft later() -> int { give 4; }", RV_CHECK_SYNTAX, NULL},
        {"craft sum(n: int) -> int { check n == 0 { give 0; } otherwise { give n + sum(n - 1); } } craft later() -> int { give sum(3); } launch() { show(later()); }", RV_CHECK_OK, NULL},
        {"craft a() -> int { give b(); } craft b() -> int { give a(); } launch() {}", RV_CHECK_OK, NULL},
        {"craft f(x: int) -> int { give x; } launch() { show(f(1.0)); }", RV_CHECK_TYPE, "argument type"},
        {"craft f(x: int) -> int { give x; } launch() { show(f()); }", RV_CHECK_TYPE, "argument count"},
        {"craft f() -> void {} launch() { keep value = f(); }", RV_CHECK_TYPE, "void"},
        {"craft f() -> void {} launch() { keep value = f; }", RV_CHECK_TYPE, "first-class"},
        {"launch() { keep value = show; }", RV_CHECK_TYPE, "first-class"},
        {"launch() { keep value = 1; value(); }", RV_CHECK_TYPE, "not a function"},
        {"launch() { missing(); }", RV_CHECK_NAME, "unknown function"},
        {"launch() { keep value = missing; }", RV_CHECK_NAME, "unknown name"},
    };
    return test_semantic_cases(cases, sizeof(cases) / sizeof(cases[0]));
}

static bool builtin_signatures(void) {
    const SemanticCase cases[] = {
        {"launch() { show(); show(ask(int), ask(real), ask(bool), ask(text)); show(toInt(1.0), toInt(\"1\"), toReal(1), toReal(\"1\"), toText(1), toText(1.0), toText(true), toBool(\"true\")); }", RV_CHECK_OK, NULL},
        {"launch() { show(toInt(1)); }", RV_CHECK_TYPE, "conversion"},
        {"launch() { show(toReal(1.0)); }", RV_CHECK_TYPE, "conversion"},
        {"launch() { show(toText(\"1\")); }", RV_CHECK_TYPE, "conversion"},
        {"launch() { show(toBool(true)); }", RV_CHECK_TYPE, "conversion"},
        {"launch() { show(toInt()); }", RV_CHECK_TYPE, "argument count"},
        {"launch() { show(toReal(1, 2)); }", RV_CHECK_TYPE, "argument count"},
        {"launch() { show([1]); }", RV_CHECK_TYPE, "primitive"},
        {"launch() { show(show()); }", RV_CHECK_TYPE, "primitive"},
        {"launch() { show(length([1, 2])); }", RV_CHECK_OK, NULL},
        {"launch() { show(length(\"hi\")); }", RV_CHECK_TYPE, "array"},
        {"launch() { keep f = fileOpen(\"tmp\", \"anything\"); show(fileWrite(\"tmp\", \"a\"), fileAppend(\"tmp\", \"b\"), fileRead(\"tmp\"), fileExists(\"tmp\"), fileReadFrom(f), fileWriteTo(f, \"c\"), fileClose(f)); }", RV_CHECK_OK, NULL},
        {"launch() { show(fileWrite(1, \"a\")); }", RV_CHECK_TYPE, "file operation"},
        {"launch() { show(fileAppend(\"a\", 1)); }", RV_CHECK_TYPE, "file operation"},
        {"launch() { show(fileRead(1)); }", RV_CHECK_TYPE, "file operation"},
        {"launch() { show(fileExists(1)); }", RV_CHECK_TYPE, "file operation"},
        {"launch() { keep f = fileOpen(\"a\", 1); }", RV_CHECK_TYPE, "file operation"},
        {"launch() { show(fileReadFrom(\"a\")); }", RV_CHECK_TYPE, "file operation"},
        {"launch() { show(fileWriteTo(\"a\", \"b\")); }", RV_CHECK_TYPE, "file operation"},
        {"launch() { show(fileClose(\"a\")); }", RV_CHECK_TYPE, "file operation"},
        {"launch() { fileClose(); }", RV_CHECK_TYPE, "argument count"},
    };
    return test_semantic_cases(cases, sizeof(cases) / sizeof(cases[0]));
}

static bool numeric_values(void) {
    RvCheckedProgram program;
    CHECK(test_accept("launch() { keep a = 010; keep b = -(2147483648); keep c = 0.25; }", &program));
    CHECK(test_statement(&program, 0)->as.declaration.initializer->number.integer == 10);
    RvAstExpr *negative = test_statement(&program, 1)->as.declaration.initializer;
    CHECK(negative->as.unary.operand->as.group->number.integer == INT64_C(2147483648));
    CHECK(test_statement(&program, 2)->as.declaration.initializer->number.real == 0.25);
    rv_checked_program_destroy(&program);

    const SemanticCase cases[] = {
        {"keep minimum = -2147483648; launch() { show(minimum, 2147483647, --2147483648); }", RV_CHECK_OK, NULL},
        {"launch() { show(2147483648); }", RV_CHECK_TYPE, "32-bit"},
        {"launch() { show(-2147483649); }", RV_CHECK_TYPE, "32-bit"},
        {"launch() { show(-(2147483648 + 0)); }", RV_CHECK_TYPE, "32-bit"},
        {"launch() { choose 1 { when 2147483648 => {} } }", RV_CHECK_TYPE, "32-bit"},
    };
    CHECK(test_semantic_cases(cases, sizeof(cases) / sizeof(cases[0])));

    char overflow[512];
    char underflow[512];
    strcpy(overflow, "launch() { keep value = ");
    size_t prefix = strlen(overflow);
    memset(overflow + prefix, '9', 340);
    strcpy(overflow + prefix + 340, ".0; }");
    strcpy(underflow, "launch() { keep value = 0.");
    prefix = strlen(underflow);
    memset(underflow + prefix, '0', 340);
    strcpy(underflow + prefix + 340, "1; }");
    const SemanticCase extremes[] = {
        {overflow, RV_CHECK_TYPE, "finite double"},
        {underflow, RV_CHECK_TYPE, "finite double"},
    };
    return test_semantic_cases(extremes, sizeof(extremes) / sizeof(extremes[0]));
}

bool test_semantic_expressions(void) {
    return operators_and_calls() && builtin_signatures() && numeric_values();
}
