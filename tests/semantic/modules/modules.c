#define _XOPEN_SOURCE 700

#include "../test.h"

#include <string.h>
#include <unistd.h>

static bool import_identities(void) {
    TestProject project;
    CHECK(test_project_init(&project));
    CHECK(test_project_write(&project, "main.ruv",
        "bring { left } from \"parts/left.ruv\";"
        "bring { right } from \"parts/right.ruv\";"
        "bring { Item } from \"shared.data\";"
        "launch() { keep value: Item = left(); show(right(value)); }"
    ));
    CHECK(test_project_write(&project, "parts/left.ruv",
        "bring { Item, amount } from \"../shared.data\";"
        "craft helper() -> int { give amount; }"
        "share craft left() -> Item { give Item { count: helper() }; }"
    ));
    CHECK(test_project_write(&project, "parts/right.ruv",
        "bring { Item } from \".././alias.data\";"
        "craft helper() -> int { give 2; }"
        "share craft right(value: Item) -> int { give value.count + helper(); }"
    ));
    CHECK(test_project_write(&project, "shared.data",
        "share object Item { count: int; } share keep amount = 10;"
    ));
    char alias[256];
    CHECK(snprintf(alias, sizeof(alias), "%s/alias.data", project.directory) > 0);
    CHECK(symlink("shared.data", alias) == 0);

    RvCheckedProgram program;
    RvCheckError error;
    bool success = rv_check_program(project.entry, rv_allocator_system(), &program, &error);
    if (!success) {
        rv_check_error_write(stderr, &error);
    }
    CHECK(success);
    CHECK(program.graph.modules.length == 4);
    RvSymbol *record = test_symbol(&program, "Item", RV_SYMBOL_RECORD);
    RvSymbol *left = test_symbol(&program, "left", RV_SYMBOL_FUNCTION);
    RvSymbol *right = test_symbol(&program, "right", RV_SYMBOL_FUNCTION);
    CHECK(record != NULL && left != NULL && right != NULL);
    CHECK(left->type.record == record && right->parameters->type.record == record);
    RvImportEdge *left_import = left->module->imports;
    RvImportEdge *right_import = right->module->imports;
    CHECK(left_import->target == right_import->target);
    CHECK(left_import->target == record->module);
    CHECK(test_statement(&program, 0)->as.declaration.initializer->symbol == left);
    CHECK(rv_module_graph_at(&program.graph, 4) == NULL);
    rv_checked_program_destroy(&program);
    CHECK(test_project_remove(&project));
    return true;
}

static bool module_errors(void) {
    const struct {
        const char *entry;
        const char *module;
        RvCheckErrorKind kind;
        const char *message;
    } cases[] = {
        {"bring { value } from \"lib.ruv\"; launch() {}", "keep value = 1;", RV_CHECK_IMPORT, "private"},
        {"bring { missing } from \"lib.ruv\"; launch() {}", "share keep value = 1;", RV_CHECK_IMPORT, "not declared"},
        {"bring { value, value } from \"lib.ruv\"; launch() {}", "share keep value = 1;", RV_CHECK_NAME, "duplicate"},
        {"bring { value } from \"lib.ruv\"; keep value = 1; launch() {}", "share keep value = 1;", RV_CHECK_NAME, "duplicate"},
        {"bring { value } from \"lib.ruv\"; launch() {}", "launch() {}", RV_CHECK_SYNTAX, "launch"},
        {"bring { value } from \"lib.ruv\"; launch() {}", "share keep value = missing;", RV_CHECK_SYNTAX, NULL},
        {"bring { value } from \"lib.ruv\"; launch() {}", "share keep value = 1; craft bad() -> int {}", RV_CHECK_RETURN, "every path"},
        {"bring { value } from \"lib.ruv\"; launch() {}", "bring { other } from \"main.ruv\"; share keep value = 1;", RV_CHECK_IMPORT, "circular"},
        {"bring { value } from \"./main.ruv\"; launch() {}", "", RV_CHECK_IMPORT, "circular"},
        {"bring { value } from \"missing.ruv\"; launch() {}", "", RV_CHECK_IO, "resolve"},
        {"bring { value } from \"/absolute.ruv\"; launch() {}", "", RV_CHECK_IMPORT, "relative"},
        {"bring { value } from \"C:/absolute.ruv\"; launch() {}", "", RV_CHECK_IMPORT, "relative"},
        {"bring { value } from \"\"; launch() {}", "", RV_CHECK_IMPORT, "relative"},
        {"bring { value } from \"lib.ruv\"; launch() {}", "bring { value } from \"other.ruv\";", RV_CHECK_IMPORT, "not declared"},
    };
    for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        TestProject project;
        CHECK(test_project_init(&project));
        CHECK(test_project_write(&project, "main.ruv", cases[index].entry));
        CHECK(test_project_write(&project, "lib.ruv", cases[index].module));
        CHECK(test_project_write(&project, "other.ruv", "share keep value = 1;"));
        TestAllocator allocation = {0};
        RvCheckedProgram program;
        RvCheckError error;
        CHECK(!rv_check_program(project.entry, test_allocator(&allocation), &program, &error));
        bool matches = error.kind == cases[index].kind;
        if (cases[index].message != NULL) {
            matches = matches && strstr(error.message, cases[index].message) != NULL;
        }
        if (!matches) {
            fprintf(stderr, "module case %zu\n", index);
            rv_check_error_write(stderr, &error);
        }
        CHECK(error.span.source != NULL);
        CHECK(error.span.end <= rv_source_length(error.span.source));
        rv_checked_program_destroy(&program);
        CHECK(allocation.live == 0);
        CHECK(test_project_remove(&project));
        CHECK(matches);
    }
    return true;
}

static bool import_depth_limit(void) {
    TestProject project;
    CHECK(test_project_init(&project));
    CHECK(test_project_write(&project, "main.ruv", "bring { x } from \"m0\"; launch() {}"));
    for (size_t index = 0; index < 130; ++index) {
        char name[32];
        char source[128];
        CHECK(snprintf(name, sizeof(name), "m%zu", index) > 0);
        CHECK(snprintf(source, sizeof(source),
                       "bring { x } from \"m%zu\"; share keep x = 1;", index + 1) > 0);
        CHECK(test_project_write(&project, name, source));
    }
    RvModuleGraph graph;
    RvCheckError error;
    CHECK(!rv_module_graph_load(project.entry, rv_allocator_system(), &graph, &error));
    CHECK(error.kind == RV_CHECK_LIMIT && error.span.source != NULL);
    rv_module_graph_destroy(&graph);
    CHECK(test_project_remove(&project));
    return true;
}

bool test_semantic_modules(void) {
    return import_identities() && module_errors() && import_depth_limit();
}
