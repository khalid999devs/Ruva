#include "../internal.h"

typedef struct Snapshot {
    RvCValue key;
    RvCValue value;
    struct Snapshot *next;
} Snapshot;

static RvCValue snapshot_value(RvCEmitter *emitter, RvCValue record,
                               const RvField *field, RvSpan span) {
    const char *helper = "rv_text_retain";
    switch (field->type.kind) {
        case RV_TYPE_INT:
            helper = "rv_text_from_int";
            break;
        case RV_TYPE_REAL:
            helper = "rv_text_from_real";
            break;
        case RV_TYPE_BOOL:
            helper = "rv_text_from_bool";
            break;
        default:
            break;
    }
    RvCValue value = rv_c_new_value(emitter, RV_TYPE_TEXT);
    rv_c_write(emitter, "%s(rt, rv_t_%zu.rv_m_%zu, ", helper, record.id, field->index);
    rv_c_location(emitter, span);
    rv_c_write(emitter, ");\n");
    rv_c_own_temporary(emitter, value, span);
    return value;
}

static void record_walk(RvCEmitter *emitter, const RvAstStmt *statement, RvCValue record) {
    Snapshot *snapshots = NULL;
    Snapshot **tail = &snapshots;
    for (RvField *field = record.record->fields; field != NULL; field = field->next) {
        Snapshot *snapshot = rv_c_new(emitter, sizeof(*snapshot), statement->span);
        if (snapshot == NULL) {
            return;
        }
        snapshot->key = rv_c_new_value(emitter, RV_TYPE_TEXT);
        rv_c_write(emitter, "rv_text_create(rt, ");
        rv_c_string(emitter, field->name->spelling, field->name->length);
        rv_c_write(emitter, ", %zu, ", field->name->length);
        rv_c_location(emitter, statement->span);
        rv_c_write(emitter, ");\n");
        rv_c_own_temporary(emitter, snapshot->key, statement->span);
        snapshot->value = snapshot_value(emitter, record, field, statement->span);
        *tail = snapshot;
        tail = &snapshot->next;
    }
    rv_c_discard(emitter, record);
    for (Snapshot *snapshot = snapshots; snapshot != NULL; snapshot = snapshot->next) {
        RvCProjection projection = {
            .symbol = statement->as.each_walk.binding,
            .key = snapshot->key,
            .value = snapshot->value,
            .parent = emitter->projection
        };
        emitter->projection = &projection;
        rv_c_block(emitter, statement->as.each_walk.body);
        emitter->projection = projection.parent;
    }
}

static void array_walk(RvCEmitter *emitter, const RvAstStmt *statement, RvCValue array) {
    rv_c_own_temporary(emitter, array, statement->span);
    RvSymbol *binding = statement->as.each_walk.binding;
    size_t index = ++emitter->next_temporary;
    rv_c_indent(emitter);
    rv_c_write(emitter, "for (size_t rv_i_%zu = 0; rv_i_%zu < rv_t_%zu->length; ++rv_i_%zu)\n",
               index, index, array.id, index);
    RvCScope *iteration = rv_c_scope_begin(emitter, statement->span);
    if (iteration == NULL) {
        return;
    }
    rv_c_indent(emitter);
    rv_c_write(emitter, "%s rv_v_%zu = rv_array_get(rt, rv_t_%zu, (int32_t)rv_i_%zu, ",
               rv_c_type(binding->type.kind), binding->id, array.id, index);
    rv_c_location(emitter, statement->span);
    rv_c_write(emitter, ").%s;\n", rv_c_element(binding->type.kind));
    rv_c_indent(emitter);
    rv_c_write(emitter, "(void)rv_v_%zu;\n", binding->id);
    rv_c_own_local(emitter, binding, statement->span);
    bool returned = rv_c_block(emitter, statement->as.each_walk.body);
    rv_c_scope_end(emitter, iteration, returned);
}

void rv_c_each_walk(RvCEmitter *emitter, const RvAstStmt *statement) {
    RvCScope *scope = rv_c_scope_begin(emitter, statement->span);
    if (scope == NULL) {
        return;
    }
    RvCValue iterable = rv_c_expression(emitter, statement->as.each_walk.iterable);
    if (emitter->error->kind != RV_EMIT_OK) {
        rv_c_scope_end(emitter, scope, false);
        return;
    }
    if (iterable.kind == RV_TYPE_ARRAY) {
        array_walk(emitter, statement, iterable);
    } else {
        record_walk(emitter, statement, iterable);
    }
    rv_c_scope_end(emitter, scope, false);
}
