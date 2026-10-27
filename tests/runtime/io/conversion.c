#define _XOPEN_SOURCE 700

#include "../test.h"

#include <locale.h>
#include <math.h>
#include <string.h>

#ifdef __APPLE__
#include <xlocale.h>
#endif

bool test_runtime_conversions(void) {
    RvRuntime runtime;
    rv_runtime_init(&runtime, stdout, stderr);
    RvLocation where = {"conversion.ruv", 1, 1};
    const char *integers[] = {" +0008 ", "-2147483648", "2147483647", "\t-0\r\n"};
    int32_t expected[] = {8, INT32_MIN, INT32_MAX, 0};
    for (size_t index = 0; index < 4; ++index) {
        RvText *text = rv_text_create(&runtime, integers[index], strlen(integers[index]), where);
        CHECK(rv_text_to_int(&runtime, text, where) == expected[index]);
        rv_text_release(&runtime, text);
    }
    locale_t locale = newlocale(LC_NUMERIC_MASK, "fr_FR.UTF-8", (locale_t)0);
    if (locale == (locale_t)0) {
        locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
    }
    CHECK(locale != (locale_t)0);
    locale_t previous = uselocale(locale);
    CHECK(previous != (locale_t)0);
    RvText *real = rv_text_create(&runtime, " \t+.125e+2\r ", 12, where);
    CHECK(rv_text_to_real(&runtime, real, where) == 12.5);
    CHECK(uselocale((locale_t)0) == locale);
    rv_text_release(&runtime, real);
    uselocale(previous);
    freelocale(locale);
    CHECK(rv_real_to_int(&runtime, 2147483647.75, where) == INT32_MAX);
    CHECK(rv_real_to_int(&runtime, -2147483648.75, where) == INT32_MIN);
    CHECK(rv_real_to_int(&runtime, -0.75, where) == 0);
    CHECK(rv_int_to_real(&runtime, INT32_MAX, where) == 2147483647.0);
    RvText *truth = rv_text_create(&runtime, "\vtrue\f", 6, where);
    CHECK(rv_text_to_bool(&runtime, truth, where));
    rv_text_release(&runtime, truth);
    RvText *zero = rv_text_create(&runtime, "-0", 2, where);
    CHECK(signbit(rv_text_to_real(&runtime, zero, where)));
    rv_text_release(&runtime, zero);
    CHECK(runtime.live_allocations == 0);
    return rv_runtime_finish(&runtime);
}
