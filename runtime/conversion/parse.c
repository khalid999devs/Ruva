#define _XOPEN_SOURCE 700

#include "../ruva_runtime.h"
#include "../../compiler/common/host/numeric.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *begin;
    const char *end;
} TextRange;

static bool ascii_space(char byte) {
    return byte == ' ' || byte == '\t' || byte == '\r' || byte == '\n'
        || byte == '\f' || byte == '\v';
}

static TextRange trimmed(const RvText *text) {
    TextRange range = {rv_text_bytes(text), rv_text_bytes(text) + rv_text_length(text)};
    while (range.begin < range.end && ascii_space(*range.begin)) {
        ++range.begin;
    }
    while (range.end > range.begin && ascii_space(range.end[-1])) {
        --range.end;
    }
    return range;
}

static bool digit(char byte) {
    return byte >= '0' && byte <= '9';
}

static bool decimal(TextRange range, bool real) {
    const char *cursor = range.begin;
    if (cursor < range.end && (*cursor == '+' || *cursor == '-')) {
        ++cursor;
    }
    size_t digits = 0;
    while (cursor < range.end && digit(*cursor)) {
        ++cursor;
        ++digits;
    }
    if (real && cursor < range.end && *cursor == '.') {
        ++cursor;
        while (cursor < range.end && digit(*cursor)) {
            ++cursor;
            ++digits;
        }
    }
    if (digits == 0) {
        return false;
    }
    if (real && cursor < range.end && (*cursor == 'e' || *cursor == 'E')) {
        ++cursor;
        if (cursor < range.end && (*cursor == '+' || *cursor == '-')) {
            ++cursor;
        }
        const char *exponent = cursor;
        while (cursor < range.end && digit(*cursor)) {
            ++cursor;
        }
        if (cursor == exponent) {
            return false;
        }
    }
    return cursor == range.end;
}

int32_t rv_text_to_int(RvRuntime *runtime, const RvText *text, RvLocation where) {
    TextRange range = trimmed(text);
    if (!decimal(range, false)) {
        rv_panic(runtime, "invalid integer text", where);
    }
    errno = 0;
    char *end;
    long value = strtol(range.begin, &end, 10);
    if (errno != 0 || end != range.end || value < INT32_MIN || value > INT32_MAX) {
        rv_panic(runtime, "integer text out of range", where);
    }
    return (int32_t)value;
}

double rv_text_to_real(RvRuntime *runtime, const RvText *text, RvLocation where) {
    TextRange range = trimmed(text);
    if (!decimal(range, true)) {
        rv_panic(runtime, "invalid real text", where);
    }
    char *end;
    double value;
    if (!rv_host_parse_real(range.begin, &end, &value)) {
        rv_panic(runtime, "cannot create numeric locale", where);
    }
    int parse_error = errno;
    if (parse_error != 0 || end != range.end || !isfinite(value)) {
        rv_panic(runtime, "real text out of range", where);
    }
    return value;
}

bool rv_text_to_bool(RvRuntime *runtime, const RvText *text, RvLocation where) {
    TextRange range = trimmed(text);
    size_t length = (size_t)(range.end - range.begin);
    if (length == 4 && memcmp(range.begin, "true", 4) == 0) {
        return true;
    }
    if (length == 5 && memcmp(range.begin, "false", 5) == 0) {
        return false;
    }
    rv_panic(runtime, "invalid boolean text", where);
}
