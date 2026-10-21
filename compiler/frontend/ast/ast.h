#ifndef RUVA_AST_H
#define RUVA_AST_H

#include "compiler/frontend/lexer/token.h"
#include "compiler/common/memory/arena.h"
#include "compiler/analysis/types/type.h"

#include <stdio.h>

typedef struct RvAstExpr RvAstExpr;
typedef struct RvAstStmt RvAstStmt;
typedef struct RvAstSequence RvAstSequence;

typedef enum {
    RV_AST_TYPE_INFER,
    RV_AST_TYPE_INT,
    RV_AST_TYPE_REAL,
    RV_AST_TYPE_BOOL,
    RV_AST_TYPE_TEXT,
    RV_AST_TYPE_VOID,
    RV_AST_TYPE_FILE,
    RV_AST_TYPE_NAMED
} RvAstTypeKind;

typedef struct {
    RvAstTypeKind kind;
    bool array;
    const RvToken *name;
    RvSpan span;
} RvAstType;

typedef struct RvAstExprList {
    RvAstExpr *value;
    struct RvAstExprList *next;
} RvAstExprList;

typedef struct RvAstFieldInit {
    const RvToken *name;
    RvAstExpr *value;
    RvField *field;
    struct RvAstFieldInit *next;
} RvAstFieldInit;

typedef enum {
    RV_AST_INTEGER,
    RV_AST_REAL,
    RV_AST_TEXT,
    RV_AST_BOOL,
    RV_AST_NAME,
    RV_AST_GROUP,
    RV_AST_UNARY,
    RV_AST_BINARY,
    RV_AST_CALL,
    RV_AST_INDEX,
    RV_AST_FIELD,
    RV_AST_ARRAY,
    RV_AST_RECORD,
    RV_AST_TYPED_ASK
} RvAstExprKind;

struct RvAstExpr {
    RvAstExprKind kind;
    RvSpan span;
    size_t depth;
    RvType type;
    RvSymbol *symbol;
    union {
        int64_t integer;
        double real;
    } number;
    union {
        const RvToken *atom;
        RvAstExpr *group;
        struct {
            const RvToken *operator;
            RvAstExpr *operand;
        } unary;
        struct {
            const RvToken *operator;
            RvAstExpr *left;
            RvAstExpr *right;
        } binary;
        struct {
            RvAstExpr *callee;
            RvAstExprList *arguments;
        } call;
        struct {
            RvAstExpr *base;
            RvAstExpr *index;
        } index;
        struct {
            RvAstExpr *base;
            const RvToken *name;
            RvField *resolved;
        } field;
        RvAstExprList *elements;
        struct {
            const RvToken *name;
            RvAstFieldInit *fields;
        } record;
        RvAstType ask_type;
    } as;
};

typedef struct {
    bool mutable;
    const RvToken *name;
    RvAstType type;
    RvAstExpr *initializer;
    RvSymbol *symbol;
} RvAstBinding;

typedef struct RvAstCase {
    const RvToken *label;
    int64_t integer;
    RvAstStmt *body;
    struct RvAstCase *next;
} RvAstCase;

typedef enum {
    RV_AST_BLOCK,
    RV_AST_DECLARATION,
    RV_AST_ASSIGNMENT,
    RV_AST_EXPRESSION,
    RV_AST_GIVE,
    RV_AST_CHECK,
    RV_AST_CHOOSE,
    RV_AST_RANGE_WALK,
    RV_AST_EACH_WALK,
    RV_AST_DURING
} RvAstStmtKind;

struct RvAstStmt {
    RvAstStmtKind kind;
    RvSpan span;
    union {
        RvAstSequence *statements;
        RvAstBinding declaration;
        struct {
            RvAstExpr *target;
            RvAstExpr *value;
        } assignment;
        RvAstExpr *expression;
        struct {
            RvAstExpr *condition;
            RvAstStmt *body;
            RvAstStmt *otherwise;
        } check;
        struct {
            RvAstExpr *selector;
            RvAstCase *cases;
            RvAstStmt *otherwise;
        } choose;
        struct {
            const RvToken *name;
            RvAstExpr *begin;
            RvAstExpr *end;
            RvAstStmt *body;
            RvSymbol *binding;
        } range_walk;
        struct {
            const RvToken *name;
            RvAstExpr *iterable;
            RvAstStmt *body;
            RvSymbol *binding;
        } each_walk;
        struct {
            RvAstExpr *condition;
            RvAstStmt *body;
        } during;
    } as;
};

/* Alternatives include their continuations; suffixes can be shared. */
struct RvAstSequence {
    RvAstStmt *statement;
    RvAstSequence *next;
    RvAstSequence *alternative;
};

typedef struct RvAstNameList {
    const RvToken *name;
    struct RvAstNameList *next;
} RvAstNameList;

typedef struct RvAstImport {
    RvSpan span;
    RvAstNameList *names;
    const RvToken *path;
    struct RvAstImport *next;
} RvAstImport;

typedef struct RvAstMember {
    const RvToken *name;
    RvAstType type;
    RvSymbol *symbol;
    RvField *field;
    struct RvAstMember *next;
} RvAstMember;

typedef enum {
    RV_AST_CONSTANT,
    RV_AST_FUNCTION,
    RV_AST_OBJECT,
    RV_AST_LAUNCH
} RvAstDeclKind;

typedef struct RvAstDecl {
    RvAstDeclKind kind;
    RvSpan span;
    bool shared;
    const RvToken *name;
    RvSymbol *symbol;
    union {
        RvAstBinding constant;
        struct {
            RvAstMember *parameters;
            RvAstType result;
            RvAstStmt *body;
        } function;
        RvAstMember *fields;
    } as;
    struct RvAstDecl *next;
} RvAstDecl;

typedef struct {
    RvArena storage;
    const RvSource *source;
    bool entry;
    RvSpan span;
    RvAstImport *imports;
    RvAstDecl *declarations;
    RvAstDecl *launch;
} RvAstModule;

/* The module owns nodes and token strings; spans still borrow its source. */
void rv_ast_module_destroy(RvAstModule *module);
bool rv_ast_dump(FILE *output, const RvAstModule *module);

#endif
