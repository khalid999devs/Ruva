#include "type.h"

bool rv_type_equal(RvType left, RvType right) {
    return left.kind == right.kind && left.element == right.element && left.record == right.record;
}

bool rv_type_primitive(RvType type) {
    return type.kind >= RV_TYPE_INT && type.kind <= RV_TYPE_TEXT;
}

bool rv_type_numeric(RvType type) {
    return type.kind == RV_TYPE_INT || type.kind == RV_TYPE_REAL;
}

const char *rv_type_name(RvType type) {
    static const char *const names[] = {
        "invalid", "void", "int", "real", "bool", "text", "file",
        "array", "record", "function", "record walk binding"
    };
    if (type.kind < RV_TYPE_INVALID || type.kind > RV_TYPE_PROJECTION) {
        return "invalid";
    }
    return names[type.kind];
}
