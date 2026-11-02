#include "tests/semantic/test.h"
#include "compiler/backend/c/emit.h"

#include <stdlib.h>
#include <string.h>

static bool emit_failures(void) {
    RvCheckedProgram program;
    char source[160000];
    const char prefix[] =
        "object Item { name: text; count: int; }"
        "craft record_copy(item: Item) -> Item { give item; }"
        "craft identity(value: text) -> text {"
        "check value == \"\" { give value; } otherwise {"
        "choose value { when \"keep\" => { give value; }"
        "otherwise => { keep copy = value; give copy; } } } }"
        "launch() { vary message = \"first\";";
    const char statement[] =
        "{ keep copy = message; message = message + \" second\";"
        "show(identity(copy), message, true && false);"
        "walk index from 0 until 1 { during false {} }"
        "vary items = [copy, message]; items[0] = pop(items);"
        "items = [copy]; push(items, copy);"
        "walk word in items { keep saved = word; }"
        "keep item = record_copy(Item { count: 1, name: copy });"
        "walk field in item { show(field.key, field.value); }"
        "keep number = toInt(toText(1)); keep handle = fileOpen(\"x\", \"write\");"
        "fileWriteTo(handle, toText(number)); fileClose(handle);"
        "during false { show(ask(int), ask(real), ask(bool), ask(text)); } }";
    size_t length = sizeof(prefix) - 1;
    memcpy(source, prefix, length);
    for (size_t index = 0; index < 160; ++index) {
        CHECK(length + sizeof(statement) + 10 < sizeof(source));
        memcpy(source + length, statement, sizeof(statement) - 1);
        length += sizeof(statement) - 1;
    }
    strcpy(source + length, "give; }");
    CHECK(test_accept(source, &program));
    TestAllocator baseline = {0};
    RvEmitError error;
    FILE *output = tmpfile();
    CHECK(output != NULL);
    CHECK(rv_c_emit(output, &program, test_allocator(&baseline), &error));
    CHECK(baseline.live == 0 && baseline.calls > 1);
    CHECK(ftell(output) > 0);
    CHECK(fclose(output) == 0);
    for (size_t failure = 1; failure <= baseline.calls; ++failure) {
        TestAllocator state = {.fail_at = failure};
        output = tmpfile();
        CHECK(output != NULL);
        CHECK(!rv_c_emit(output, &program, test_allocator(&state), &error));
        CHECK(error.kind == RV_EMIT_ALLOCATION && state.live == 0);
        CHECK(error.span.source != NULL && program.valid);
        CHECK(fclose(output) == 0);
    }
    output = fopen(__FILE__, "rb");
    CHECK(output != NULL);
    CHECK(!rv_c_emit(output, &program, rv_allocator_system(), &error));
    CHECK(error.kind == RV_EMIT_IO);
    CHECK(fclose(output) == 0);
    CHECK(!rv_c_emit(NULL, &program, rv_allocator_system(), &error));
    CHECK(error.kind == RV_EMIT_ARGUMENT);
    CHECK(!rv_c_supported(NULL, NULL));
    rv_checked_program_destroy(&program);
    CHECK(!rv_c_supported(&program, &error));
    CHECK(error.kind == RV_EMIT_ARGUMENT);
    return true;
}

static bool full_language_preflight(void) {
    const char *programs[] = {
        "craft f() -> void { show(ask(int)); } launch() {}",
        "launch() { show(ask(int)); }",
        "launch() { show(toText(1)); }",
        "launch() { check true { fileExists(\"x\"); } }",
        "launch() { give; during false { ask(int); } }",
    };
    for (size_t index = 0; index < sizeof(programs) / sizeof(programs[0]); ++index) {
        RvCheckedProgram program;
        CHECK(test_accept(programs[index], &program));
        FILE *output = tmpfile();
        CHECK(output != NULL);
        RvEmitError error;
        CHECK(rv_c_emit(output, &program, rv_allocator_system(), &error));
        CHECK(error.kind == RV_EMIT_OK);
        CHECK(ftell(output) > 0);
        CHECK(fclose(output) == 0);
        rv_checked_program_destroy(&program);
    }
    return true;
}

int main(void) {
    if (!emit_failures() || !full_language_preflight()) {
        return EXIT_FAILURE;
    }
    puts("backend checks passed");
    return EXIT_SUCCESS;
}
