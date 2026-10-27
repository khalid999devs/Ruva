#include "read.h"

#include <string.h>

static void reserve(RvRuntime *runtime, char **buffer, size_t *capacity,
                     size_t required, RvLocation where) {
    if (required <= *capacity) {
        return;
    }
    size_t next = *capacity == 0 ? 256 : *capacity;
    while (next < required) {
        if (next > SIZE_MAX / 2) {
            next = required;
            break;
        }
        next *= 2;
    }
    char *resized = rv_try_resize(runtime, *buffer, next);
    if (resized == NULL) {
        rv_panic(runtime, "out of memory reading text", where);
    }
    *buffer = resized;
    *capacity = next;
}

RvText *rv_read_text(RvRuntime *runtime, FILE *stream, bool line, RvLocation where) {
    char *buffer = NULL;
    size_t length = 0;
    size_t capacity = 0;
    bool received = false;
    for (;;) {
        char chunk[4096];
        size_t count;
        if (line) {
            int byte = fgetc(stream);
            if (byte == EOF) {
                break;
            }
            received = true;
            if (byte == '\n') {
                if (length != 0 && buffer[length - 1] == '\r') {
                    --length;
                }
                break;
            }
            chunk[0] = (char)(unsigned char)byte;
            count = 1;
        } else {
            count = fread(chunk, 1, sizeof(chunk), stream);
            if (count == 0) {
                break;
            }
        }
        if (count > SIZE_MAX - length) {
            rv_panic(runtime, "text is too large", where);
        }
        reserve(runtime, &buffer, &capacity, length + count, where);
        memcpy(buffer + length, chunk, count);
        length += count;
    }
    if (ferror(stream)) {
        rv_panic(runtime, "input read failed", where);
    }
    if (line && !received) {
        rv_panic(runtime, "unexpected end of input", where);
    }
    RvText *text = rv_text_create(runtime, buffer == NULL ? "" : buffer, length, where);
    rv_release(runtime, buffer);
    return text;
}
