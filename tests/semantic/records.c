#include "test.h"

static bool record_identity_and_fields(void) {
    RvCheckedProgram program;
    CHECK(test_accept(
        "object Item { show: text; switch: int; key: bool; value: real; }"
        "craft copy(item: Item) -> Item { give item; }"
        "launch() {"
        "vary item = Item { value: 2.0, key: true, switch: 7, show: \"ok\" };"
        "keep other = copy(item); item.switch = 8;"
        "show(other.show, other.switch, other.key, other.value);"
        "walk field in item { show(field.key, field.value); item.switch = 9; }"
        "}", &program
    ));
    RvAstExpr *constructor = test_statement(&program, 0)->as.declaration.initializer;
    CHECK(constructor->type.kind == RV_TYPE_RECORD);
    CHECK(constructor->type.record == test_symbol(&program, "Item", RV_SYMBOL_RECORD));
    RvAstFieldInit *field = constructor->as.record.fields;
    for (size_t index = 4; index != 0; --index) {
        CHECK(field != NULL && field->field->index == index - 1);
        field = field->next;
    }
    CHECK(field == NULL);
    RvAstStmt *walk = test_statement(&program, 4);
    CHECK(walk->kind == RV_AST_EACH_WALK);
    CHECK(walk->as.each_walk.binding->kind == RV_SYMBOL_PROJECTION);
    RvAstExpr *projection = walk->as.each_walk.body->as.statements->statement
        ->as.expression->as.call.arguments->value;
    CHECK(projection->type.kind == RV_TYPE_TEXT);
    CHECK(projection->symbol == walk->as.each_walk.binding);
    rv_checked_program_destroy(&program);

    const SemanticCase cases[] = {
        {"object A { x: int; } object B { x: int; } launch() { vary a = A { x: 1 }; a = B { x: 2 }; }", RV_CHECK_TYPE, "assignment"},
        {"object A { x: int; } launch() { keep a = A {}; }", RV_CHECK_TYPE, "every field"},
        {"object A { x: int; } launch() { keep a = A { x: 1, x: 2 }; }", RV_CHECK_TYPE, "duplicate constructor"},
        {"object A { x: int; } launch() { keep a = A { y: 1 }; }", RV_CHECK_TYPE, "unknown record field"},
        {"object A { x: int; } launch() { keep a = A { x: 1.0 }; }", RV_CHECK_TYPE, "initializer type"},
        {"object A { x: int; x: text; } launch() {}", RV_CHECK_NAME, "duplicate record"},
        {"object A {} launch() { keep a = A {}; show(a); }", RV_CHECK_TYPE, "primitive"},
        {"object A {} launch() { keep a = A {} == A {}; }", RV_CHECK_TYPE, "operator"},
        {"object A {} launch() { keep a = A; }", RV_CHECK_TYPE, "first-class"},
        {"launch() { keep a = Missing {}; }", RV_CHECK_TYPE, "constructor"},
        {"object A {} launch() { keep a = A {}; show(a.missing); }", RV_CHECK_TYPE, "unknown record field"},
        {"object A {} launch() { walk field in A {} { keep escaped = field; } }", RV_CHECK_TYPE, "first-class"},
        {"object A {} launch() { walk field in A {} { show(field.other); } }", RV_CHECK_TYPE, "key and value"},
        {"object A {} launch() { walk field in A {} { field.key = \"x\"; } }", RV_CHECK_OWNERSHIP, "read-only"},
        {"object A { x: int; } craft f(a: A) -> void { a.x = 2; } launch() {}", RV_CHECK_OWNERSHIP, "read-only"},
        {"object A { x: int; } launch() { keep a = A { x: 1 }; a.x = 2; }", RV_CHECK_OWNERSHIP, "read-only"},
        {"object A { x: int; } craft f(a: A) -> void { vary b = a; b.x = 2; } launch() { f(A { x: 1 }); }", RV_CHECK_OK, NULL},
    };
    return test_semantic_cases(cases, sizeof(cases) / sizeof(cases[0]));
}

static bool return_coverage(void) {
    const SemanticCase cases[] = {
        {"craft f(x: bool) -> int { check x { give 1; } otherwise { give 2; } } launch() {}", RV_CHECK_OK, NULL},
        {"craft f(x: int) -> text { choose x { when 1 => { give \"one\"; } otherwise => { give \"other\"; } } } launch() {}", RV_CHECK_OK, NULL},
        {"craft f() -> int { { give 1; } } launch() {}", RV_CHECK_OK, NULL},
        {"craft f() -> int { during false {} give 1; } launch() {}", RV_CHECK_OK, NULL},
        {"craft f() -> void { give; } launch() { give; }", RV_CHECK_OK, NULL},
        {"craft f() -> int {} launch() {}", RV_CHECK_RETURN, "every path"},
        {"craft f(x: bool) -> int { check x { give 1; } } launch() {}", RV_CHECK_RETURN, "every path"},
        {"craft f(x: int) -> int { choose x { when 1 => { give 1; } } } launch() {}", RV_CHECK_RETURN, "every path"},
        {"craft f(x: int) -> int { choose x { when 1 => {} otherwise => { give 2; } } } launch() {}", RV_CHECK_RETURN, "every path"},
        {"craft f() -> int { during true { give 1; } } launch() {}", RV_CHECK_RETURN, "every path"},
        {"craft f() -> int { walk i from 0 until 2 { give 1; } } launch() {}", RV_CHECK_RETURN, "every path"},
        {"craft f() -> int { give; } launch() {}", RV_CHECK_RETURN, "must return a value"},
        {"craft f() -> int { give 1.0; } launch() {}", RV_CHECK_RETURN, "type mismatch"},
        {"craft f() -> void { give 1; } launch() {}", RV_CHECK_RETURN, "void function"},
        {"launch() { give 1; }", RV_CHECK_RETURN, "void function"},
        {"craft f() -> int { give 1; show(missing); } launch() {}", RV_CHECK_NAME, "unknown name"},
    };
    return test_semantic_cases(cases, sizeof(cases) / sizeof(cases[0]));
}

bool test_semantic_records(void) {
    return record_identity_and_fields() && return_coverage();
}
