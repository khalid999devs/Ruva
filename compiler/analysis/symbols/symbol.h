#ifndef RUVA_SYMBOL_H
#define RUVA_SYMBOL_H

#include "compiler/analysis/types/type.h"
#include "compiler/frontend/lexer/token.h"

typedef struct RvModuleUnit RvModuleUnit;
typedef struct RvAstDecl RvAstDecl;

typedef enum {
    RV_BUILTIN_NONE,
    RV_BUILTIN_ASK,
    RV_BUILTIN_SHOW,
    RV_BUILTIN_TO_INT,
    RV_BUILTIN_TO_REAL,
    RV_BUILTIN_TO_TEXT,
    RV_BUILTIN_TO_BOOL,
    RV_BUILTIN_PUSH,
    RV_BUILTIN_POP,
    RV_BUILTIN_LENGTH,
    RV_BUILTIN_FILE_WRITE,
    RV_BUILTIN_FILE_APPEND,
    RV_BUILTIN_FILE_READ,
    RV_BUILTIN_FILE_EXISTS,
    RV_BUILTIN_FILE_OPEN,
    RV_BUILTIN_FILE_READ_FROM,
    RV_BUILTIN_FILE_WRITE_TO,
    RV_BUILTIN_FILE_CLOSE,
    RV_BUILTIN_COUNT
} RvBuiltin;

typedef enum {
    RV_SYMBOL_BUILTIN,
    RV_SYMBOL_CONSTANT,
    RV_SYMBOL_FUNCTION,
    RV_SYMBOL_RECORD,
    RV_SYMBOL_LOCAL,
    RV_SYMBOL_PARAMETER,
    RV_SYMBOL_LOOP,
    RV_SYMBOL_PROJECTION
} RvSymbolKind;

struct RvField {
    const RvToken *name;
    RvType type;
    size_t index;
    RvField *next;
};

typedef struct RvParameter {
    const RvToken *name;
    RvType type;
    RvSymbol *binding;
    struct RvParameter *next;
} RvParameter;

struct RvSymbol {
    size_t id;
    RvSymbolKind kind;
    const char *name;
    const RvToken *token;
    RvType type;
    bool mutable;
    RvBuiltin builtin;
    RvModuleUnit *module;
    RvAstDecl *declaration;
    RvField *fields;
    RvParameter *parameters;
};

#endif
