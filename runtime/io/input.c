#include "read.h"

RvText *rv_ask_text(RvRuntime *runtime, RvLocation where) {
    return rv_read_text(runtime, runtime->input, true, where);
}

int32_t rv_ask_int(RvRuntime *runtime, RvLocation where) {
    RvText *text = rv_ask_text(runtime, where);
    int32_t value = rv_text_to_int(runtime, text, where);
    rv_text_release(runtime, text);
    return value;
}

double rv_ask_real(RvRuntime *runtime, RvLocation where) {
    RvText *text = rv_ask_text(runtime, where);
    double value = rv_text_to_real(runtime, text, where);
    rv_text_release(runtime, text);
    return value;
}

bool rv_ask_bool(RvRuntime *runtime, RvLocation where) {
    RvText *text = rv_ask_text(runtime, where);
    bool value = rv_text_to_bool(runtime, text, where);
    rv_text_release(runtime, text);
    return value;
}
