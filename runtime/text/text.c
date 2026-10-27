#include "internal.h"

#include <string.h>

static bool valid_text(const unsigned char *bytes, size_t length) {
    size_t index = 0;
    while (index < length) {
        unsigned char first = bytes[index++];
        if (first == 0) {
            return false;
        }
        if (first < 0x80) {
            continue;
        }
        size_t continuation;
        uint32_t value;
        uint32_t minimum;
        if (first >= 0xc2 && first <= 0xdf) {
            continuation = 1;
            value = first & 0x1f;
            minimum = 0x80;
        } else if (first >= 0xe0 && first <= 0xef) {
            continuation = 2;
            value = first & 0x0f;
            minimum = 0x800;
        } else if (first >= 0xf0 && first <= 0xf4) {
            continuation = 3;
            value = first & 0x07;
            minimum = 0x10000;
        } else {
            return false;
        }
        if (continuation > length - index) {
            return false;
        }
        for (size_t count = 0; count < continuation; ++count) {
            unsigned char byte = bytes[index++];
            if ((byte & 0xc0) != 0x80) {
                return false;
            }
            value = (value << 6) | (byte & 0x3f);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) {
            return false;
        }
    }
    return true;
}

static RvText *new_text(RvRuntime *runtime, size_t length, RvLocation where) {
    if (length > SIZE_MAX - sizeof(RvText) - 1) {
        rv_panic(runtime, "text is too large", where);
    }
    RvText *text = rv_allocate(runtime, sizeof(*text) + length + 1, where);
    text->references = 1;
    text->length = length;
    text->bytes[length] = '\0';
    return text;
}

RvText *rv_text_create(RvRuntime *runtime, const char *bytes, size_t length, RvLocation where) {
    if (!valid_text((const unsigned char *)bytes, length)) {
        rv_panic(runtime, "invalid UTF-8 text", where);
    }
    RvText *text = new_text(runtime, length, where);
    memcpy(text->bytes, bytes, length);
    return text;
}

RvText *rv_text_retain(RvRuntime *runtime, RvText *text, RvLocation where) {
    if (text->references == SIZE_MAX) {
        rv_panic(runtime, "text reference count overflow", where);
    }
    ++text->references;
    return text;
}

void rv_text_release(RvRuntime *runtime, RvText *text) {
    if (--text->references == 0) {
        rv_release(runtime, text);
    }
}

RvText *rv_text_concat(RvRuntime *runtime, const RvText *left, const RvText *right,
                       RvLocation where) {
    if (right->length > SIZE_MAX - left->length) {
        rv_panic(runtime, "text is too large", where);
    }
    RvText *text = new_text(runtime, left->length + right->length, where);
    memcpy(text->bytes, left->bytes, left->length);
    memcpy(text->bytes + left->length, right->bytes, right->length);
    return text;
}

bool rv_text_equal(const RvText *left, const RvText *right) {
    return rv_text_equal_bytes(left, right->bytes, right->length);
}

bool rv_text_equal_bytes(const RvText *text, const char *bytes, size_t length) {
    return text->length == length && memcmp(text->bytes, bytes, length) == 0;
}

const char *rv_text_bytes(const RvText *text) {
    return text->bytes;
}

size_t rv_text_length(const RvText *text) {
    return text->length;
}
