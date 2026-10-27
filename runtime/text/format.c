#define _XOPEN_SOURCE 700

#include "../ruva_runtime.h"
#include "../../compiler/common/host/numeric.h"

#include <math.h>

size_t rv_format_real(RvRuntime *runtime, char *buffer, size_t capacity,
                       double value, RvLocation where) {
    const char *special = NULL;
    if (isnan(value)) {
        special = "nan";
    } else if (isinf(value)) {
        special = signbit(value) ? "-inf" : "inf";
    }
    int written;
    if (special != NULL) {
        written = snprintf(buffer, capacity, "%s", special);
    } else {
        written = rv_host_format_real(buffer, capacity, "%.17g", value);
    }
    if (written < 0 || (size_t)written >= capacity) {
        rv_panic(runtime, "cannot format real value", where);
    }
    return (size_t)written;
}
