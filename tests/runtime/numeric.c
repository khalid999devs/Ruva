#define _XOPEN_SOURCE 700

#include "test.h"

#include <locale.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#ifdef __APPLE__
#include <xlocale.h>
#endif

bool test_runtime_numbers(void) {
    RvRuntime runtime;
    FILE *output = tmpfile();
    CHECK(output != NULL);
    rv_runtime_init(&runtime, output, stderr);
    RvLocation where = {"numbers.ruv", 2, 4};
    CHECK(rv_int_add(&runtime, INT32_MAX, 0, where) == INT32_MAX);
    CHECK(rv_int_subtract(&runtime, INT32_MIN, 0, where) == INT32_MIN);
    CHECK(rv_int_multiply(&runtime, INT32_MIN, 1, where) == INT32_MIN);
    CHECK(rv_int_divide(&runtime, -7, 3, where) == -2);
    CHECK(rv_int_remainder(&runtime, -7, 3, where) == -1);
    CHECK(rv_int_remainder(&runtime, INT32_MIN, -1, where) == 0);
    CHECK(rv_int_negate(&runtime, -1, where) == 1);
    CHECK(rv_real_divide(&runtime, 1.0, 4.0, where) == 0.25);
    char buffer[32];
    CHECK(rv_format_real(&runtime, buffer, sizeof(buffer), 250.0 / 3.0, where) > 0);
    CHECK(strcmp(buffer, "83.333333333333329") == 0);
    rv_format_real(&runtime, buffer, sizeof(buffer), INFINITY, where);
    CHECK(strcmp(buffer, "inf") == 0);
    rv_format_real(&runtime, buffer, sizeof(buffer), -INFINITY, where);
    CHECK(strcmp(buffer, "-inf") == 0);
    rv_format_real(&runtime, buffer, sizeof(buffer), NAN, where);
    CHECK(strcmp(buffer, "nan") == 0);
    rv_format_real(&runtime, buffer, sizeof(buffer), -0.0, where);
    CHECK(strcmp(buffer, "-0") == 0);

    const char *locales[] = {"de_DE.UTF-8", "fr_FR.UTF-8", "C"};
    locale_t locale = (locale_t)0;
    for (size_t index = 0; index < 3 && locale == (locale_t)0; ++index) {
        locale = newlocale(LC_NUMERIC_MASK, locales[index], (locale_t)0);
    }
    CHECK(locale != (locale_t)0);
    locale_t previous = uselocale(locale);
    CHECK(previous != (locale_t)0);
    rv_format_real(&runtime, buffer, sizeof(buffer), 1.25, where);
    CHECK(strcmp(buffer, "1.25") == 0 && uselocale((locale_t)0) == locale);
    uselocale(previous);
    freelocale(locale);

    rv_print_int(&runtime, INT32_MIN, where);
    rv_print_separator(&runtime, where);
    rv_print_real(&runtime, 1.25, where);
    rv_print_separator(&runtime, where);
    rv_print_bool(&runtime, false, where);
    rv_print_separator(&runtime, where);
    RvText *text = rv_text_create(&runtime, "%s", 2, where);
    rv_print_text(&runtime, text, where);
    rv_text_release(&runtime, text);
    rv_print_newline(&runtime, where);
    CHECK(rv_runtime_finish(&runtime));
    CHECK(test_stream_equals(output, "-2147483648 1.25 false %s\n"));
    CHECK(fclose(output) == 0);
    FILE *errors = tmpfile();
    output = tmpfile();
    CHECK(output != NULL && errors != NULL);
    rv_runtime_init(&runtime, output, errors);
    RvText *pending = rv_text_create(&runtime, "buffered", 8, where);
    rv_print_text(&runtime, pending, where);
    CHECK(close(fileno(output)) == 0);
    CHECK(!rv_runtime_finish(&runtime));
    CHECK(runtime.live_allocations == 0);
    CHECK(test_stream_equals(errors, "numbers.ruv:2:4: error: output failed\n"));
    CHECK(fclose(output) == EOF);
    CHECK(fclose(errors) == 0);
    return true;
}
