#include "test.h"

#include <string.h>

bool test_runtime_text(void) {
    RvRuntime runtime;
    rv_runtime_init(&runtime, stdout, stderr);
    RvLocation where = {"text.ruv", 1, 1};
    RvText *first = rv_text_create(&runtime, "hello", 5, where);
    RvText *shared = rv_text_retain(&runtime, first, where);
    RvText *second = rv_text_create(&runtime, "hello", 5, where);
    CHECK(first != second && shared == first && rv_text_equal(first, second));
    rv_text_release(&runtime, first);
    CHECK(strcmp(rv_text_bytes(shared), "hello") == 0);
    CHECK(rv_text_equal_bytes(shared, "hello", 5));
    CHECK(!rv_text_equal_bytes(shared, "hell", 4));
    CHECK(!rv_text_equal_bytes(shared, "world", 5));
    RvText *joined = rv_text_concat(&runtime, shared, second, where);
    CHECK(rv_text_length(joined) == 10 && strcmp(rv_text_bytes(joined), "hellohello") == 0);
    rv_text_release(&runtime, joined);
    rv_text_release(&runtime, shared);
    rv_text_release(&runtime, second);
    CHECK(runtime.live_allocations == 0);

    const char unicode[] = "\xc2\xa2\xe2\x82\xac\xf0\x9f\x98\x80";
    RvText *utf8 = rv_text_create(&runtime, unicode, sizeof(unicode) - 1, where);
    CHECK(rv_text_length(utf8) == sizeof(unicode) - 1);
    rv_text_release(&runtime, utf8);
    RvText *current = rv_text_create(&runtime, "", 0, where);
    char chunk[100];
    memset(chunk, 'x', sizeof(chunk));
    RvText *suffix = rv_text_create(&runtime, chunk, sizeof(chunk), where);
    for (size_t index = 0; index < 1000; ++index) {
        RvText *next = rv_text_concat(&runtime, current, suffix, where);
        rv_text_release(&runtime, current);
        current = next;
        CHECK(runtime.live_allocations == 2);
        CHECK(runtime.live_bytes < rv_text_length(current) + 1024);
    }
    CHECK(rv_text_length(current) == 100000);
    rv_text_release(&runtime, current);
    rv_text_release(&runtime, suffix);
    CHECK(runtime.live_allocations == 0);
    return rv_runtime_finish(&runtime);
}
