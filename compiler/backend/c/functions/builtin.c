#include "../internal.h"

const char *rv_c_builtin_helper(RvBuiltin builtin, RvTypeKind input) {
    switch (builtin) {
        case RV_BUILTIN_TO_INT:
            return input == RV_TYPE_TEXT ? "rv_text_to_int" : "rv_real_to_int";
        case RV_BUILTIN_TO_REAL:
            return input == RV_TYPE_TEXT ? "rv_text_to_real" : "rv_int_to_real";
        case RV_BUILTIN_TO_TEXT:
            switch (input) {
                case RV_TYPE_INT:
                    return "rv_text_from_int";
                case RV_TYPE_REAL:
                    return "rv_text_from_real";
                default:
                    return "rv_text_from_bool";
            }
        case RV_BUILTIN_TO_BOOL:
            return "rv_text_to_bool";
        case RV_BUILTIN_FILE_WRITE:
            return "rv_file_write";
        case RV_BUILTIN_FILE_APPEND:
            return "rv_file_append";
        case RV_BUILTIN_FILE_READ:
            return "rv_file_read";
        case RV_BUILTIN_FILE_EXISTS:
            return "rv_file_exists";
        case RV_BUILTIN_FILE_OPEN:
            return "rv_file_open";
        case RV_BUILTIN_FILE_READ_FROM:
            return "rv_file_read_from";
        case RV_BUILTIN_FILE_WRITE_TO:
            return "rv_file_write_to";
        case RV_BUILTIN_FILE_CLOSE:
            return "rv_file_close";
        default:
            return NULL;
    }
}

RvCValue rv_c_ask(RvCEmitter *emitter, const RvAstExpr *expression) {
    const char *helper = "rv_ask_text";
    switch (expression->type.kind) {
        case RV_TYPE_INT:
            helper = "rv_ask_int";
            break;
        case RV_TYPE_REAL:
            helper = "rv_ask_real";
            break;
        case RV_TYPE_BOOL:
            helper = "rv_ask_bool";
            break;
        default:
            break;
    }
    RvCValue value = rv_c_new_value(emitter, expression->type.kind);
    rv_c_write(emitter, "%s(rt, ", helper);
    rv_c_location(emitter, expression->span);
    rv_c_write(emitter, ");\n");
    return value;
}
