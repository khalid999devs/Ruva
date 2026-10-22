#include "../test.h"

#include <stdlib.h>
#include <string.h>

static bool allocation_failures(void) {
    TestProject project;
    CHECK(test_project_init(&project));
    CHECK(test_project_write(&project, "main.ruv",
        "bring { Item, make } from \"lib.ruv\";"
        "launch() { vary items: int[] = []; push(items, 1);"
        "keep item: Item = make(1.25);"
        "walk x in items { { vary items = [x]; push(items, 2); } }"
        "walk field in item { show(field.key, field.value); } }"
    ));
    CHECK(test_project_write(&project, "lib.ruv",
        "share object Item { count: int; label: text; }"
        "share craft make(n: real) -> Item {"
        "check n > 0.0 { give Item { label: toText(n), count: 1 }; }"
        "otherwise { give Item { count: 0, label: \"empty\" }; } }"
    ));
    TestAllocator baseline = {0};
    RvCheckedProgram program;
    RvCheckError error;
    CHECK(rv_check_program(project.entry, test_allocator(&baseline), &program, &error));
    size_t calls = baseline.calls;
    rv_checked_program_destroy(&program);
    CHECK(baseline.live == 0 && calls > 20);

    for (size_t failure = 1; failure <= calls; ++failure) {
        TestAllocator allocation = {.fail_at = failure};
        CHECK(!rv_check_program(project.entry, test_allocator(&allocation), &program, &error));
        if (error.kind != RV_CHECK_ALLOCATION) {
            fprintf(stderr, "allocation %zu/%zu\n", failure, calls);
            rv_check_error_write(stderr, &error);
        }
        CHECK(error.kind == RV_CHECK_ALLOCATION);
        CHECK(!program.valid);
        FILE *diagnostic = tmpfile();
        CHECK(diagnostic != NULL);
        CHECK(rv_check_error_write(diagnostic, &error));
        CHECK(fclose(diagnostic) == 0);
        rv_checked_program_destroy(&program);
        CHECK(allocation.live == 0);
    }
    CHECK(test_project_remove(&project));
    return true;
}

static bool large_flat_body(void) {
    size_t capacity = 40000;
    char *source = malloc(capacity);
    CHECK(source != NULL);
    const char prefix[] = "launch() { vary value = 0;";
    const char statement[] = "value = value + 1;";
    size_t length = sizeof(prefix) - 1;
    memcpy(source, prefix, length);
    for (size_t index = 0; index < 2000; ++index) {
        CHECK(length + sizeof(statement) + 2 < capacity);
        memcpy(source + length, statement, sizeof(statement) - 1);
        length += sizeof(statement) - 1;
    }
    strcpy(source + length, "}");
    RvCheckedProgram program;
    CHECK(test_accept(source, &program));
    size_t count = 0;
    RvAstSequence *sequence = program.graph.entry->ast.launch->as.function.body->as.statements;
    while (sequence != NULL) {
        ++count;
        sequence = sequence->next;
    }
    CHECK(count == 2001);
    rv_checked_program_destroy(&program);
    free(source);
    return true;
}

static bool invalid_api(void) {
    RvCheckedProgram program = {0};
    RvCheckError error;
    RvAllocator allocator = rv_allocator_system();
    CHECK(!rv_check_program(NULL, allocator, &program, &error));
    CHECK(error.kind == RV_CHECK_ARGUMENT);
    rv_checked_program_destroy(&program);
    CHECK(!rv_check_program("", allocator, &program, &error));
    CHECK(error.kind == RV_CHECK_ARGUMENT);
    rv_checked_program_destroy(&program);
    CHECK(!rv_check_program("missing", allocator, NULL, &error));
    CHECK(error.kind == RV_CHECK_ARGUMENT);
    CHECK(!rv_module_graph_load("missing", allocator, NULL, &error));
    CHECK(error.kind == RV_CHECK_ARGUMENT);
    CHECK(!rv_check_program(NULL, allocator, &program, NULL));
    rv_checked_program_destroy(&program);

    const SemanticCase cases[] = {
        {"launch() { show(@); }", RV_CHECK_LEXICAL, NULL},
        {"launch() { keep x = ; }", RV_CHECK_SYNTAX, NULL},
    };
    return test_semantic_cases(cases, sizeof(cases) / sizeof(cases[0]));
}

static bool mutated_input(void) {
    const char seed[] =
        "object Empty {} craft f(x: int) -> int { give x; }"
        "launch() { vary values = [1, 2]; keep ready = true;"
        "check ready {} {} walk item in values { show(f(item)); }"
        "walk field in Empty {} {} }";
    const char replacements[] = "{}();:=.,+-![]012aq \n";
    TestProject project;
    CHECK(test_project_init(&project));
    uint32_t random = 37;
    for (size_t index = 0; index < 256; ++index) {
        char source[sizeof(seed)];
        memcpy(source, seed, sizeof(seed));
        random = random * UINT32_C(1664525) + UINT32_C(1013904223);
        size_t position = random % (sizeof(seed) - 1);
        source[position] = replacements[(random >> 16) % (sizeof(replacements) - 1)];
        CHECK(test_project_write(&project, "main.ruv", source));
        TestAllocator allocation = {0};
        RvCheckedProgram program;
        RvCheckError error;
        bool success = rv_check_program(project.entry, test_allocator(&allocation), &program, &error);
        CHECK(success == program.valid);
        CHECK(success == (error.kind == RV_CHECK_OK));
        if (!success) {
            FILE *diagnostic = tmpfile();
            CHECK(diagnostic != NULL);
            CHECK(rv_check_error_write(diagnostic, &error));
            CHECK(fclose(diagnostic) == 0);
        }
        rv_checked_program_destroy(&program);
        CHECK(allocation.live == 0);
    }
    CHECK(test_project_remove(&project));
    return true;
}

bool test_semantic_failures(void) {
    return allocation_failures() && large_flat_body() && invalid_api() && mutated_input();
}
