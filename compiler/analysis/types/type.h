#ifndef RUVA_TYPE_H
#define RUVA_TYPE_H

#include <stdbool.h>

typedef struct RvSymbol RvSymbol;
typedef struct RvField RvField;

typedef enum {
    RV_TYPE_INVALID,
    RV_TYPE_VOID,
    RV_TYPE_INT,
    RV_TYPE_REAL,
    RV_TYPE_BOOL,
    RV_TYPE_TEXT,
    RV_TYPE_FILE,
    RV_TYPE_ARRAY,
    RV_TYPE_RECORD,
    RV_TYPE_FUNCTION,
    RV_TYPE_PROJECTION
} RvTypeKind;

typedef struct {
    RvTypeKind kind;
    RvTypeKind element;
    RvSymbol *record;
} RvType;

bool rv_type_equal(RvType left, RvType right);
bool rv_type_primitive(RvType type);
bool rv_type_numeric(RvType type);
const char *rv_type_name(RvType type);

#endif
