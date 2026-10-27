#include "../test.h"

#include <string.h>

bool test_runtime_input(void) {
    FILE *input = tmpfile();
    CHECK(input != NULL);
    const char lines[] = " +0008 \r\n 1.25e2\n\vtrue\f\n  spaced text  \r\n\nlast\r";
    CHECK(fwrite(lines, 1, sizeof(lines) - 1, input) == sizeof(lines) - 1);
    rewind(input);
    RvRuntime runtime;
    rv_runtime_init(&runtime, stdout, stderr);
    runtime.input = input;
    RvLocation where = {"input.ruv", 1, 1};
    CHECK(rv_ask_int(&runtime, where) == 8);
    CHECK(rv_ask_real(&runtime, where) == 125.0);
    CHECK(rv_ask_bool(&runtime, where));
    RvText *spaced = rv_ask_text(&runtime, where);
    CHECK(strcmp(rv_text_bytes(spaced), "  spaced text  ") == 0);
    rv_text_release(&runtime, spaced);
    RvText *empty = rv_ask_text(&runtime, where);
    CHECK(rv_text_length(empty) == 0);
    rv_text_release(&runtime, empty);
    RvText *last = rv_ask_text(&runtime, where);
    CHECK(strcmp(rv_text_bytes(last), "last\r") == 0);
    rv_text_release(&runtime, last);
    CHECK(runtime.live_allocations == 0);
    CHECK(fclose(input) == 0);
    return rv_runtime_finish(&runtime);
}
