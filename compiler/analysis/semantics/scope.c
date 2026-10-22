#include "internal.h"

#include <string.h>

static size_t name_bucket(const char *name) {
    uint64_t hash = UINT64_C(14695981039346656037);
    for (const unsigned char *byte = (const unsigned char *)name; *byte != 0; ++byte) {
        hash ^= *byte;
        hash *= UINT64_C(1099511628211);
    }
    return (size_t)(hash % 32);
}

RvScope *rv_check_scope(RvChecker *checker, RvScope *parent, RvSpan span) {
    RvScope *scope = rv_check_new(checker, sizeof(*scope), span);
    if (scope != NULL) {
        scope->parent = parent;
    }
    return scope;
}

RvSymbol *rv_scope_local(const RvScope *scope, const char *name) {
    for (RvScopeBinding *binding = scope->buckets[name_bucket(name)];
         binding != NULL; binding = binding->next) {
        if (strcmp(binding->name, name) == 0) {
            return binding->symbol;
        }
    }
    return NULL;
}

RvSymbol *rv_scope_lookup(const RvScope *scope, const char *name) {
    for (const RvScope *current = scope; current != NULL; current = current->parent) {
        RvSymbol *symbol = rv_scope_local(current, name);
        if (symbol != NULL) {
            return symbol;
        }
    }
    return NULL;
}

bool rv_scope_insert(RvChecker *checker, RvScope *scope, const char *name,
                      RvSymbol *symbol, RvSpan span) {
    if (rv_scope_local(scope, name) != NULL) {
        return rv_check_fail(checker, RV_CHECK_NAME, span, "duplicate name in this scope");
    }

    RvScopeBinding *binding = rv_check_new(checker, sizeof(*binding), span);
    if (binding == NULL) {
        return false;
    }
    size_t bucket = name_bucket(name);
    binding->name = name;
    binding->symbol = symbol;
    binding->next = scope->buckets[bucket];
    scope->buckets[bucket] = binding;
    return true;
}

RvSymbol *rv_check_symbol(RvChecker *checker, RvSymbolKind kind, const RvToken *name) {
    RvSpan span = name == NULL ? (RvSpan){0} : name->span;
    RvSymbol *symbol = rv_check_new(checker, sizeof(*symbol), span);
    if (symbol == NULL) {
        return NULL;
    }

    RvSymbol **slot = rv_vector_append(&checker->program->symbols);
    if (slot == NULL) {
        rv_check_fail(checker, RV_CHECK_ALLOCATION, span, "out of memory while checking source");
        return NULL;
    }

    *slot = symbol;
    symbol->id = checker->program->symbols.length;
    symbol->kind = kind;
    symbol->token = name;
    symbol->name = name == NULL ? NULL : name->spelling;
    symbol->module = checker->unit;
    return symbol;
}

bool rv_check_define(RvChecker *checker, RvSymbol *symbol) {
    if (rv_scope_local(checker->builtins, symbol->name) != NULL) {
        return rv_check_fail(checker, RV_CHECK_NAME, symbol->token->span,
                             "built-in names cannot be redefined or shadowed");
    }
    return rv_scope_insert(
        checker, checker->scope, symbol->name, symbol, symbol->token->span
    );
}

const char *rv_builtin_name(RvBuiltin builtin) {
    static const char *const names[] = {
        "", "ask", "show", "toInt", "toReal", "toText", "toBool", "push", "pop", "length",
        "fileWrite", "fileAppend", "fileRead", "fileExists", "fileOpen",
        "fileReadFrom", "fileWriteTo", "fileClose"
    };
    if (builtin <= RV_BUILTIN_NONE || builtin >= RV_BUILTIN_COUNT) {
        return "";
    }
    return names[builtin];
}

bool rv_check_builtins(RvChecker *checker) {
    checker->builtins = rv_check_scope(checker, NULL, (RvSpan){0});
    if (checker->builtins == NULL) {
        return false;
    }

    for (RvBuiltin builtin = RV_BUILTIN_ASK; builtin < RV_BUILTIN_COUNT; ++builtin) {
        RvSymbol *symbol = rv_check_symbol(checker, RV_SYMBOL_BUILTIN, NULL);
        if (symbol == NULL) {
            return false;
        }
        symbol->name = rv_builtin_name(builtin);
        symbol->builtin = builtin;
        symbol->type.kind = RV_TYPE_FUNCTION;
        if (!rv_scope_insert(checker, checker->builtins, symbol->name, symbol, (RvSpan){0})) {
            return false;
        }
    }
    return true;
}
