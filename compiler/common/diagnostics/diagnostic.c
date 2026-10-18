#include "diagnostic.h"

bool rv_diagnostic_write(FILE *stream, RvSpan span, const char *message) {
    RvSourcePosition position;
    if (stream == NULL || message == NULL || span.source == NULL
        || span.start > span.end || span.end > rv_source_length(span.source)
        || !rv_source_position(span.source, span.start, &position)) {
        return false;
    }
    return fprintf(stream, "%s:%zu:%zu: error: %s\n", rv_source_path(span.source),
                   position.line, position.column, message) >= 0;
}

bool rv_diagnostic_load_error(FILE *stream, const char *path, RvSourceError error) {
    if (stream == NULL || path == NULL) {
        return false;
    }
    const char *message;
    switch (error.kind) {
        case RV_SOURCE_INVALID: message = "invalid source input"; break;
        case RV_SOURCE_IO: message = "cannot read source"; break;
        case RV_SOURCE_ALLOCATION: message = "out of memory while loading source"; break;
        case RV_SOURCE_TOO_LARGE: message = "source is too large"; break;
        default: return false;
    }
    return fprintf(stream, "%s: error: %s\n", path, message) >= 0;
}
