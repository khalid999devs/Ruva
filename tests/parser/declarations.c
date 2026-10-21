#include "test.h"

#include <string.h>

static bool module_declarations_and_imports(void) {
    TestProgram program;
    CHECK(test_parse_program(
        "bring { Other, helper } from \"other.ruv\";"
        "bring { last } from \"last.ruv\";"
        "share keep count: int = -010;"
        "keep title = \"module\";"
        "share object Pair { return: text; amount: int; ratio: real; ready: bool; }"
        "object Empty {}"
        "share craft identity(value: Pair, times: int) -> Pair { give value; }"
        "craft finish() -> void { give; }", false, &program
    ));

    CHECK(!program.module.entry && program.module.launch == NULL);
    RvAstImport *import = program.module.imports;
    CHECK(strcmp(import->names->name->spelling, "Other") == 0);
    CHECK(strcmp(import->names->next->name->spelling, "helper") == 0);
    CHECK(strcmp(import->path->text, "other.ruv") == 0);
    CHECK(strcmp(import->next->path->text, "last.ruv") == 0 && import->next->next == NULL);

    RvAstDecl *declaration = program.module.declarations;
    CHECK(declaration->kind == RV_AST_CONSTANT && declaration->shared);
    CHECK(declaration->as.constant.type.kind == RV_AST_TYPE_INT);
    CHECK(declaration->as.constant.initializer->kind == RV_AST_UNARY);
    CHECK(!declaration->as.constant.mutable);

    declaration = declaration->next;
    CHECK(declaration->as.constant.type.kind == RV_AST_TYPE_INFER && !declaration->shared);
    declaration = declaration->next;
    CHECK(declaration->kind == RV_AST_OBJECT && declaration->shared);
    CHECK(strcmp(declaration->as.fields->name->spelling, "return") == 0);
    CHECK(declaration->as.fields->type.kind == RV_AST_TYPE_TEXT);
    CHECK(declaration->as.fields->next->type.kind == RV_AST_TYPE_INT);
    CHECK(declaration->as.fields->next->next->type.kind == RV_AST_TYPE_REAL);
    CHECK(declaration->as.fields->next->next->next->type.kind == RV_AST_TYPE_BOOL);

    declaration = declaration->next;
    CHECK(declaration->kind == RV_AST_OBJECT && declaration->as.fields == NULL);
    declaration = declaration->next;
    CHECK(declaration->kind == RV_AST_FUNCTION && declaration->shared);
    CHECK(declaration->as.function.parameters->type.kind == RV_AST_TYPE_NAMED);
    CHECK(strcmp(declaration->as.function.parameters->type.name->spelling, "Pair") == 0);
    CHECK(declaration->as.function.parameters->next->type.kind == RV_AST_TYPE_INT);
    CHECK(declaration->as.function.result.kind == RV_AST_TYPE_NAMED);
    CHECK(declaration->next->as.function.result.kind == RV_AST_TYPE_VOID);
    CHECK(program.module.span.end == rv_source_length(program.source));

    test_program_destroy(&program);
    CHECK(test_parse_program("", false, &program));
    CHECK(program.module.declarations == NULL && program.module.imports == NULL);
    test_program_destroy(&program);
    return true;
}

static bool entry_structure(void) {
    TestProgram program;
    CHECK(test_parse_program(
        "object Empty {} craft work() -> int { give 1; } launch() {}", true, &program
    ));
    CHECK(program.module.entry && program.module.launch->kind == RV_AST_LAUNCH);
    CHECK(program.module.launch->as.function.result.kind == RV_AST_TYPE_VOID);
    CHECK(program.module.launch->as.function.parameters == NULL);
    CHECK(program.module.launch->as.function.body->as.statements == NULL);
    test_program_destroy(&program);
    return true;
}

static bool malformed_declarations(void) {
    const char *modules[] = {
        "bring {} from \"empty.ruv\";",
        "bring { value, } from \"other.ruv\";",
        "bring { value } \"other.ruv\";",
        "bring { value } from path;",
        "keep value = 1; bring { x } from \"late.ruv\";",
        "vary global = 1;",
        "keep value = f();",
        "keep value = 1 + 2;",
        "keep value = (1);",
        "keep value = -true;",
        "keep value: int[] = 1;",
        "keep value: Pair = 1;",
        "share launch() {}",
        "launch() {}",
        "object Pair { nested: Pair; }",
        "object Pair { values: int[]; }",
        "object Pair { handle: file; }",
        "object Pair { result: void; }",
        "object Pair { amount: int }",
        "object Pair { craft method() -> void {} }",
        "craft f(value: int[]) -> void {}",
        "craft f(value: file) -> void {}",
        "craft f(value: void) -> void {}",
        "craft f(value: int,) -> void {}",
        "craft f(value) -> void {}",
        "craft f() int {}",
        "craft f() -> int[] {}",
        "craft f() -> file {}",
    };
    for (size_t index = 0; index < sizeof(modules) / sizeof(modules[0]); ++index) {
        CHECK(test_parse_error(modules[index], false, RV_PARSE_SYNTAX, NULL));
    }

    const char *entries[] = {
        "", "keep value = 1;", "launch(value: int) {}", "launch() {} launch() {}",
        "launch() {} keep later = 1;", "launch() {};",
    };
    for (size_t index = 0; index < sizeof(entries) / sizeof(entries[0]); ++index) {
        CHECK(test_parse_error(entries[index], true, RV_PARSE_SYNTAX, NULL));
    }
    return true;
}

bool test_parser_declarations(void) {
    return module_declarations_and_imports()
        && entry_structure()
        && malformed_declarations();
}
