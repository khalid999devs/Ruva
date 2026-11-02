#include "../internal.h"

#include <string.h>

void rv_c_write_type(RvCEmitter *emitter, RvType type) {
    if (type.kind == RV_TYPE_RECORD) {
        rv_c_write(emitter, "rv_r_%zu", type.record->id);
    } else {
        rv_c_write(emitter, "%s", rv_c_type(type.kind));
    }
}

RvCValue rv_c_new_typed_value(RvCEmitter *emitter, RvType type) {
    RvCValue value = {.id = ++emitter->next_temporary, .kind = type.kind, .record = type.record};
    rv_c_indent(emitter);
    rv_c_write_type(emitter, type);
    rv_c_write(emitter, " rv_t_%zu = ", value.id);
    return value;
}

void rv_c_release_named(RvCEmitter *emitter, RvType type, const char *prefix, size_t id) {
    if (type.kind == RV_TYPE_TEXT) {
        rv_c_indent(emitter);
        rv_c_write(emitter, "rv_text_release(rt, %s%zu);\n", prefix, id);
    } else if (type.kind == RV_TYPE_ARRAY) {
        rv_c_indent(emitter);
        rv_c_write(emitter, "rv_array_release(rt, %s%s%zu);\n",
                   strcmp(prefix, "rv_v_") == 0 ? "&" : "", prefix, id);
    } else if (type.kind == RV_TYPE_RECORD) {
        for (RvField *field = type.record->fields; field != NULL; field = field->next) {
            if (field->type.kind == RV_TYPE_TEXT) {
                rv_c_indent(emitter);
                rv_c_write(emitter, "rv_text_release(rt, %s%zu.rv_m_%zu);\n", prefix, id, field->index);
            }
        }
    }
}

void rv_c_retain_record(RvCEmitter *emitter, RvCValue value, RvSpan span) {
    if (value.kind != RV_TYPE_RECORD) {
        return;
    }
    for (RvField *field = value.record->fields; field != NULL; field = field->next) {
        if (field->type.kind == RV_TYPE_TEXT) {
            rv_c_indent(emitter);
            rv_c_write(emitter, "rv_text_retain(rt, rv_t_%zu.rv_m_%zu, ", value.id, field->index);
            rv_c_location(emitter, span);
            rv_c_write(emitter, ");\n");
        }
    }
}
