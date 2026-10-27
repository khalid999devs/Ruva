#include "../test.h"
#include "runtime/text/internal.h"

#include <string.h>

bool test_runtime_text_failure(RvRuntime *runtime, const char *mode, RvLocation where) {
    if (strcmp(mode, "reference-overflow") == 0) {
        RvText *text = rv_text_create(runtime, "live", 4, where);
        text->references = SIZE_MAX;
        rv_text_retain(runtime, text, where);
    } else if (strcmp(mode, "concat-overflow") == 0) {
        RvText *left = rv_text_create(runtime, "left", 4, where);
        RvText *right = rv_text_create(runtime, "right", 5, where);
        left->length = SIZE_MAX;
        rv_text_concat(runtime, left, right, where);
    } else {
        return false;
    }
    return true;
}
