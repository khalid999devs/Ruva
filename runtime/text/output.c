#include "../ruva_runtime.h"

#include <inttypes.h>
#include <string.h>

static void write_bytes(RvRuntime *runtime, const char *bytes, size_t length, RvLocation where) {
    runtime->output_location = where;
    if (fwrite(bytes, 1, length, runtime->output) != length) {
        rv_panic(runtime, "output failed", where);
    }
}

void rv_print_int(RvRuntime *runtime, int32_t value, RvLocation where) {
    char buffer[16];
    int length = snprintf(buffer, sizeof(buffer), "%" PRId32, value);
    if (length < 0 || (size_t)length >= sizeof(buffer)) {
        rv_panic(runtime, "cannot format integer value", where);
    }
    write_bytes(runtime, buffer, (size_t)length, where);
}

void rv_print_real(RvRuntime *runtime, double value, RvLocation where) {
    char buffer[32];
    size_t length = rv_format_real(runtime, buffer, sizeof(buffer), value, where);
    write_bytes(runtime, buffer, length, where);
}

void rv_print_bool(RvRuntime *runtime, bool value, RvLocation where) {
    const char *bytes = value ? "true" : "false";
    write_bytes(runtime, bytes, strlen(bytes), where);
}

void rv_print_text(RvRuntime *runtime, const RvText *value, RvLocation where) {
    write_bytes(runtime, rv_text_bytes(value), rv_text_length(value), where);
}

void rv_print_separator(RvRuntime *runtime, RvLocation where) {
    write_bytes(runtime, " ", 1, where);
}

void rv_print_newline(RvRuntime *runtime, RvLocation where) {
    write_bytes(runtime, "\n", 1, where);
}
