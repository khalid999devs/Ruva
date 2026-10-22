#include "../internal.h"

RvType rv_check_record(RvChecker *checker, RvAstExpr *expression) {
    const RvToken *name = expression->as.record.name;
    RvSymbol *record = rv_scope_lookup(checker->scope, name->spelling);
    if (record == NULL || record->kind != RV_SYMBOL_RECORD) {
        rv_check_fail(checker, RV_CHECK_TYPE, name->span, "constructor name does not denote a record type");
        return (RvType){0};
    }

    size_t count = 0;
    for (RvField *field = record->fields; field != NULL; field = field->next) {
        ++count;
    }
    if (count == 0) {
        if (expression->as.record.fields != NULL) {
            rv_check_fail(checker, RV_CHECK_TYPE, expression->as.record.fields->name->span,
                          "unknown record field");
            return (RvType){0};
        }
        expression->symbol = record;
        return record->type;
    }
    size_t bytes;
    if (!rv_size_multiply(count, sizeof(bool), &bytes)) {
        rv_check_fail(checker, RV_CHECK_LIMIT, expression->span, "record has too many fields");
        return (RvType){0};
    }
    bool *seen = rv_check_new(checker, bytes, expression->span);
    if (seen == NULL) {
        return (RvType){0};
    }

    size_t initialized = 0;
    for (RvAstFieldInit *item = expression->as.record.fields; item != NULL; item = item->next) {
        RvField *field = rv_check_field(record, item->name->spelling);
        if (field == NULL) {
            rv_check_fail(checker, RV_CHECK_TYPE, item->name->span, "unknown record field");
            return (RvType){0};
        }
        if (seen[field->index]) {
            rv_check_fail(checker, RV_CHECK_TYPE, item->name->span, "duplicate constructor field");
            return (RvType){0};
        }
        RvType actual = rv_check_expression(checker, item->value, field->type);
        if (actual.kind == RV_TYPE_INVALID) {
            return actual;
        }
        if (!rv_type_equal(actual, field->type)) {
            rv_check_fail(checker, RV_CHECK_TYPE, item->value->span, "record field initializer type mismatch");
            return (RvType){0};
        }
        seen[field->index] = true;
        item->field = field;
        ++initialized;
    }

    if (initialized != count) {
        rv_check_fail(checker, RV_CHECK_TYPE, expression->span,
                      "record constructor must initialize every field");
        return (RvType){0};
    }
    expression->symbol = record;
    return record->type;
}
