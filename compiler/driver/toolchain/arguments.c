#include "host.h"
#include "compiler/common/memory/allocator.h"

#include <stdlib.h>
#include <string.h>

char *rv_toolchain_command_line(char *const *arguments) {
    size_t capacity = 1;
    for (size_t index = 0; arguments[index] != NULL; ++index) {
        size_t doubled;
        if (!rv_size_multiply(strlen(arguments[index]), 2, &doubled)
            || !rv_size_add(doubled, 3, &doubled)
            || !rv_size_add(capacity, doubled, &capacity)) {
            return NULL;
        }
    }
    char *line = malloc(capacity);
    if (line == NULL) {
        return NULL;
    }
    char *output = line;
    for (size_t index = 0; arguments[index] != NULL; ++index) {
        if (index != 0) {
            *output++ = ' ';
        }
        *output++ = '"';
        const char *input = arguments[index];
        for (;;) {
            size_t slashes = 0;
            while (*input == '\\') {
                ++slashes;
                ++input;
            }
            /* The Windows C startup parser doubles slashes before quotes and the closing quote. */
            size_t written = (*input == '"' || *input == '\0') ? slashes * 2 : slashes;
            for (size_t slash = 0; slash < written; ++slash) {
                *output++ = '\\';
            }
            if (*input == '\0') {
                break;
            }
            if (*input == '"') {
                *output++ = '\\';
            }
            *output++ = *input++;
        }
        *output++ = '"';
    }
    *output = '\0';
    return line;
}
