#include "test.h"

#include <string.h>

static bool scopes_and_protected_names(void) {
    RvCheckedProgram program;
    CHECK(test_accept(
        "keep value = 1; craft f(value: int) -> int {"
        "{ keep value = value + 1; show(value); } give value; }"
        "launch() { keep key = f(value); keep rv_private = key; show(rv_private); }", &program
    ));
    RvSymbol *constant = test_symbol(&program, "value", RV_SYMBOL_CONSTANT);
    RvSymbol *parameter = test_symbol(&program, "value", RV_SYMBOL_PARAMETER);
    RvSymbol *local = test_symbol(&program, "value", RV_SYMBOL_LOCAL);
    CHECK(constant != NULL && parameter != NULL && local != NULL);
    CHECK(constant->id != parameter->id && parameter->id != local->id);
    RvAstStmt *inner = parameter->module->ast.declarations->next->as.function.body
        ->as.statements->statement;
    RvAstExpr *initializer = inner->as.statements->statement->as.declaration.initializer;
    CHECK(initializer->as.binary.left->symbol == parameter);
    CHECK(rv_checked_symbol_at(&program, program.symbols.length) == NULL);
    rv_checked_program_destroy(&program);

    const SemanticCase cases[] = {
        {"keep x = 1; keep x = 2; launch() {}", RV_CHECK_NAME, "duplicate"},
        {"launch() { keep x = 1; keep x = 2; }", RV_CHECK_NAME, "duplicate"},
        {"craft f(x: int, x: int) -> void {} launch() {}", RV_CHECK_NAME, "duplicate parameter"},
        {"craft f(x: int) -> void { keep x = 2; } launch() {}", RV_CHECK_NAME, "duplicate"},
        {"launch() { walk x in [1] { keep x = 2; } }", RV_CHECK_NAME, "duplicate"},
        {"launch() { show(x); keep x = 1; }", RV_CHECK_NAME, "unknown name"},
        {"launch() { { keep x = 1; } show(x); }", RV_CHECK_NAME, "unknown name"},
    };
    CHECK(test_semantic_cases(cases, sizeof(cases) / sizeof(cases[0])));

    const char *builtins[] = {
        "ask", "show", "toInt", "toReal", "toText", "toBool", "push", "pop", "length",
        "fileWrite", "fileAppend", "fileRead", "fileExists", "fileOpen", "fileReadFrom",
        "fileWriteTo", "fileClose"
    };
    const char *forms[] = {
        "keep %s = 1; launch() {}",
        "launch() { keep %s = 1; }",
        "craft f(%s: int) -> void {} launch() {}",
        "launch() { walk %s in [1] {} }",
        "object %s {} launch() {}",
        "craft %s() -> void {} launch() {}"
    };
    for (size_t name = 0; name < sizeof(builtins) / sizeof(builtins[0]); ++name) {
        for (size_t form = 0; form < sizeof(forms) / sizeof(forms[0]); ++form) {
            char source[256];
            int length = snprintf(source, sizeof(source), forms[form], builtins[name]);
            CHECK(length > 0 && (size_t)length < sizeof(source));
            SemanticCase item = {source, RV_CHECK_NAME, "built-in"};
            CHECK(test_semantic_cases(&item, 1));
        }
    }
    return true;
}

static bool deferred_headers(void) {
    RvCheckedProgram program;
    CHECK(test_accept(
        "object Empty {} launch() {"
        "walk field in Empty {} {}"
        "keep Empty = true; check Empty {} { show(1); }"
        "during Empty {} {}"
        "keep count = 1; choose count {} {}"
        "walk index from 0 until count {} {}"
        "}", &program
    ));
    RvAstSequence *sequence = program.graph.entry->ast.launch->as.function.body->as.statements;
    CHECK(sequence->alternative == NULL);
    CHECK(sequence->statement->as.each_walk.iterable->kind == RV_AST_RECORD);
    CHECK(sequence->next->statement->kind == RV_AST_DECLARATION);
    sequence = sequence->next->next;
    CHECK(sequence->alternative == NULL);
    CHECK(sequence->statement->as.check.condition->kind == RV_AST_NAME);
    CHECK(sequence->next->statement->kind == RV_AST_BLOCK);
    for (; sequence != NULL; sequence = sequence->next) {
        CHECK(sequence->alternative == NULL);
    }
    rv_checked_program_destroy(&program);

    const SemanticCase cases[] = {
        {"object Empty {} launch() { check Empty {} {} }", RV_CHECK_TYPE, "bool"},
        {"object Empty {} launch() { keep Empty = true; walk field in Empty {} {} }", RV_CHECK_TYPE, "walk"},
        {"launch() { keep ready = true; check ready {} {} otherwise {} }", RV_CHECK_TYPE, "constructor"},
        {"launch() { check missing {} {} }", RV_CHECK_NAME, "unknown name"},
        {"object Empty {} launch() { walk Empty in Empty {} { show(Empty.key); } }", RV_CHECK_OK, NULL},
    };
    CHECK(test_semantic_cases(cases, sizeof(cases) / sizeof(cases[0])));

    char source[16000];
    const char prefix[] = "launch() { keep ready = true;";
    const char header[] = "check ready {} {} ";
    size_t length = sizeof(prefix) - 1;
    memcpy(source, prefix, length);
    for (size_t index = 0; index < 400; ++index) {
        memcpy(source + length, header, sizeof(header) - 1);
        length += sizeof(header) - 1;
    }
    strcpy(source + length, "}");
    CHECK(test_accept(source, &program));
    size_t count = 0;
    sequence = program.graph.entry->ast.launch->as.function.body->as.statements;
    for (; sequence != NULL; sequence = sequence->next) {
        CHECK(sequence->alternative == NULL);
        ++count;
    }
    CHECK(count == 801);
    rv_checked_program_destroy(&program);
    return true;
}

bool test_semantic_names(void) {
    return scopes_and_protected_names() && deferred_headers();
}
