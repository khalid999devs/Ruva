#include "../ruva_runtime.h"

#include <math.h>

int32_t rv_real_to_int(RvRuntime *runtime, double value, RvLocation where) {
    if (!isfinite(value) || value <= -2147483649.0 || value >= 2147483648.0) {
        rv_panic(runtime, "real value cannot convert to int", where);
    }
    return (int32_t)value;
}

double rv_int_to_real(RvRuntime *runtime, int32_t value, RvLocation where) {
    (void)runtime;
    (void)where;
    return (double)value;
}
