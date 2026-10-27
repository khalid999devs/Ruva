#include "../ruva_runtime.h"

static int32_t checked(RvRuntime *runtime, int64_t value, RvLocation where) {
    if (value < INT32_MIN || value > INT32_MAX) {
        rv_panic(runtime, "integer overflow", where);
    }
    return (int32_t)value;
}

int32_t rv_int_add(RvRuntime *runtime, int32_t left, int32_t right, RvLocation where) {
    return checked(runtime, (int64_t)left + right, where);
}

int32_t rv_int_subtract(RvRuntime *runtime, int32_t left, int32_t right, RvLocation where) {
    return checked(runtime, (int64_t)left - right, where);
}

int32_t rv_int_multiply(RvRuntime *runtime, int32_t left, int32_t right, RvLocation where) {
    return checked(runtime, (int64_t)left * right, where);
}

int32_t rv_int_divide(RvRuntime *runtime, int32_t left, int32_t right, RvLocation where) {
    if (right == 0) {
        rv_panic(runtime, "division by zero", where);
    }
    if (left == INT32_MIN && right == -1) {
        rv_panic(runtime, "integer overflow", where);
    }
    return left / right;
}

int32_t rv_int_remainder(RvRuntime *runtime, int32_t left, int32_t right, RvLocation where) {
    if (right == 0) {
        rv_panic(runtime, "remainder by zero", where);
    }
    return (int32_t)((int64_t)left % (int64_t)right);
}

int32_t rv_int_negate(RvRuntime *runtime, int32_t value, RvLocation where) {
    return checked(runtime, -(int64_t)value, where);
}

double rv_real_divide(RvRuntime *runtime, double left, double right, RvLocation where) {
    if (right == 0.0) {
        rv_panic(runtime, "division by zero", where);
    }
    return left / right;
}
