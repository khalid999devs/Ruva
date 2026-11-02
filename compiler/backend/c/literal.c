#include "internal.h"

void rv_c_string(RvCEmitter *emitter, const char *bytes, size_t length) {
    rv_c_write(emitter, "\"");
    for (size_t index = 0; index < length; ++index) {
        if (index != 0 && index % 24 == 0) {
            rv_c_write(emitter, "\"\n");
            rv_c_indent(emitter);
            rv_c_write(emitter, "\"");
        }
        rv_c_write(emitter, "\\%03o", (unsigned int)(unsigned char)bytes[index]);
    }
    rv_c_write(emitter, "\"");
}

void rv_c_location(RvCEmitter *emitter, RvSpan span) {
    RvSourcePosition position;
    rv_source_position(span.source, span.start, &position);
    const char *path = rv_source_path(span.source);
    size_t length = 0;
    while (path[length] != '\0') {
        ++length;
    }
    rv_c_write(emitter, "(RvLocation){");
    rv_c_string(emitter, path, length);
    rv_c_write(emitter, ", %zu, %zu}", position.line, position.column);
}

const char *rv_c_type(RvTypeKind kind) {
    switch (kind) {
        case RV_TYPE_INT:
            return "int32_t";
        case RV_TYPE_REAL:
            return "double";
        case RV_TYPE_BOOL:
            return "bool";
        case RV_TYPE_FILE:
            return "RvFile *";
        case RV_TYPE_ARRAY:
            return "RvArray *";
        case RV_TYPE_TEXT:
            return "RvText *";
        default:
            return "void";
    }
}
