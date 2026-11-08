#include "test.h"
#include "compiler/driver/toolchain/host.h"

#include <stdlib.h>
#include <string.h>

bool test_host_arguments(void) {
    const struct {
        char *argument;
        const char *expected;
    } cases[] = {
        {"", "\"\""},
        {"gcc", "\"gcc\""},
        {"two words", "\"two words\""},
        {"a\"b", "\"a\\\"b\""},
        {"C:\\folder\\", "\"C:\\folder\\\\\""},
        {"a\\\"b", "\"a\\\\\\\"b\""},
        {"a&b;$(literal)", "\"a&b;$(literal)\""},
        {"বাংলা", "\"বাংলা\""},
    };
    for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        char *arguments[] = {cases[index].argument, NULL};
        char *line = rv_toolchain_command_line(arguments);
        CHECK(line != NULL);
        CHECK(strcmp(line, cases[index].expected) == 0);
        free(line);
    }
    char *arguments[] = {"compiler with spaces", "", "-o", "C:\\output\\", NULL};
    char *line = rv_toolchain_command_line(arguments);
    CHECK(line != NULL);
    CHECK(strcmp(line, "\"compiler with spaces\" \"\" \"-o\" \"C:\\output\\\\\"") == 0);
    free(line);
    return true;
}
