#include "../ruva_runtime.h"

#include <inttypes.h>

RvText *rv_text_from_int(RvRuntime *runtime, int32_t value, RvLocation where) {
    char buffer[32];
    int length = snprintf(buffer, sizeof(buffer), "%" PRId32, value);
    if (length < 0 || (size_t)length >= sizeof(buffer)) {
        rv_panic(runtime, "cannot format integer", where);
    }
    return rv_text_create(runtime, buffer, (size_t)length, where);
}

RvText *rv_text_from_real(RvRuntime *runtime, double value, RvLocation where) {
    char buffer[64];
    size_t length = rv_format_real(runtime, buffer, sizeof(buffer), value, where);
    return rv_text_create(runtime, buffer, length, where);
}

RvText *rv_text_from_bool(RvRuntime *runtime, bool value, RvLocation where) {
    const char *bytes = value ? "true" : "false";
    size_t length = value ? 4 : 5;
    return rv_text_create(runtime, bytes, length, where);
}
