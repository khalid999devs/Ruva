#include "internal.h"

#include <string.h>

RvType rv_check_type(RvChecker *checker, RvAstType type) {
    RvType result = {0};
    switch (type.kind) {
        case RV_AST_TYPE_INFER:
            return result;
        case RV_AST_TYPE_INT:
            result.kind = RV_TYPE_INT;
            break;
        case RV_AST_TYPE_REAL:
            result.kind = RV_TYPE_REAL;
            break;
        case RV_AST_TYPE_BOOL:
            result.kind = RV_TYPE_BOOL;
            break;
        case RV_AST_TYPE_TEXT:
            result.kind = RV_TYPE_TEXT;
            break;
        case RV_AST_TYPE_VOID:
            result.kind = RV_TYPE_VOID;
            break;
        case RV_AST_TYPE_FILE:
            result.kind = RV_TYPE_FILE;
            break;
        case RV_AST_TYPE_NAMED: {
            RvSymbol *symbol = rv_scope_lookup(checker->scope, type.name->spelling);
            if (symbol == NULL || symbol->kind != RV_SYMBOL_RECORD) {
                rv_check_fail(checker, RV_CHECK_TYPE, type.span, "name does not denote a record type");
                return result;
            }
            result = (RvType){.kind = RV_TYPE_RECORD, .record = symbol};
            break;
        }
    }

    if (type.array) {
        if (!rv_type_primitive(result)) {
            rv_check_fail(checker, RV_CHECK_TYPE, type.span, "arrays require a primitive element type");
            return (RvType){0};
        }
        result = (RvType){.kind = RV_TYPE_ARRAY, .element = result.kind};
    }
    return result;
}

RvField *rv_check_field(RvSymbol *record, const char *name) {
    for (RvField *field = record->fields; field != NULL; field = field->next) {
        if (strcmp(field->name->spelling, name) == 0) {
            return field;
        }
    }
    return NULL;
}

static bool collect_locals(RvChecker *checker) {
    RvAstModule *module = &checker->unit->ast;
    checker->unit->scope = rv_check_scope(checker, checker->builtins, module->span);
    checker->scope = checker->unit->scope;
    if (checker->scope == NULL) {
        return false;
    }

    for (RvAstDecl *declaration = module->declarations;
         declaration != NULL; declaration = declaration->next) {
        RvSymbolKind kind = RV_SYMBOL_CONSTANT;
        if (declaration->kind == RV_AST_OBJECT) {
            kind = RV_SYMBOL_RECORD;
        } else if (declaration->kind == RV_AST_FUNCTION) {
            kind = RV_SYMBOL_FUNCTION;
        }

        RvSymbol *symbol = rv_check_symbol(checker, kind, declaration->name);
        if (symbol == NULL || !rv_check_define(checker, symbol)) {
            return false;
        }
        symbol->declaration = declaration;
        declaration->symbol = symbol;
        if (kind == RV_SYMBOL_RECORD) {
            symbol->type = (RvType){.kind = RV_TYPE_RECORD, .record = symbol};
        }
    }
    return true;
}

static bool bind_imports(RvChecker *checker) {
    for (RvImportEdge *edge = checker->unit->imports; edge != NULL; edge = edge->next) {
        for (RvAstNameList *name = edge->syntax->names; name != NULL; name = name->next) {
            RvSymbol *symbol = rv_scope_local(edge->target->scope, name->name->spelling);
            if (symbol == NULL || symbol->module != edge->target) {
                return rv_check_fail(checker, RV_CHECK_IMPORT, name->name->span,
                                     "imported name is not declared by this module");
            }
            if (!symbol->declaration->shared) {
                return rv_check_fail(checker, RV_CHECK_IMPORT, name->name->span,
                                     "imported declaration is private");
            }
            if (!rv_scope_insert(checker, checker->scope, name->name->spelling, symbol, name->name->span)) {
                return false;
            }
        }
    }
    return true;
}

static bool record_fields(RvChecker *checker, RvAstDecl *declaration) {
    RvSymbol *symbol = declaration->symbol;
    RvField **tail = &symbol->fields;
    size_t index = 0;

    for (RvAstMember *member = declaration->as.fields; member != NULL; member = member->next) {
        if (rv_check_field(symbol, member->name->spelling) != NULL) {
            return rv_check_fail(checker, RV_CHECK_NAME, member->name->span, "duplicate record field");
        }
        RvType type = rv_check_type(checker, member->type);
        if (!rv_type_primitive(type)) {
            return rv_check_fail(checker, RV_CHECK_TYPE, member->type.span,
                                 "record fields must have primitive types");
        }

        RvField *field = rv_check_new(checker, sizeof(*field), member->name->span);
        if (field == NULL) {
            return false;
        }
        field->name = member->name;
        field->type = type;
        field->index = index++;
        member->field = field;
        *tail = field;
        tail = &field->next;
    }
    return true;
}

static bool function_signature(RvChecker *checker, RvAstDecl *declaration) {
    RvSymbol *function = declaration->symbol;
    function->type = rv_check_type(checker, declaration->as.function.result);
    if (function->type.kind == RV_TYPE_INVALID) {
        return false;
    }

    RvParameter **tail = &function->parameters;
    for (RvAstMember *member = declaration->as.function.parameters; member != NULL; member = member->next) {
        if (rv_scope_local(checker->builtins, member->name->spelling) != NULL) {
            return rv_check_fail(checker, RV_CHECK_NAME, member->name->span,
                                 "built-in names cannot be redefined or shadowed");
        }
        for (RvParameter *prior = function->parameters; prior != NULL; prior = prior->next) {
            if (strcmp(prior->name->spelling, member->name->spelling) == 0) {
                return rv_check_fail(checker, RV_CHECK_NAME, member->name->span, "duplicate parameter");
            }
        }
        RvType type = rv_check_type(checker, member->type);
        if (type.kind == RV_TYPE_INVALID) {
            return false;
        }
        RvParameter *parameter = rv_check_new(checker, sizeof(*parameter), member->name->span);
        if (parameter == NULL) {
            return false;
        }
        parameter->name = member->name;
        parameter->type = type;
        *tail = parameter;
        tail = &parameter->next;
    }
    return true;
}

static bool constant_type(RvChecker *checker, RvAstDecl *declaration) {
    RvAstBinding *binding = &declaration->as.constant;
    RvType expected = rv_check_type(checker, binding->type);
    RvType actual = rv_check_expression(checker, binding->initializer, expected);
    if (actual.kind == RV_TYPE_INVALID) {
        return false;
    }
    if (expected.kind != RV_TYPE_INVALID && !rv_type_equal(expected, actual)) {
        return rv_check_fail(checker, RV_CHECK_TYPE, binding->initializer->span,
                             "constant initializer type does not match annotation");
    }
    declaration->symbol->type = actual;
    binding->symbol = declaration->symbol;
    return true;
}

bool rv_check_declarations(RvChecker *checker) {
    for (size_t index = 0; index < checker->program->graph.modules.length; ++index) {
        checker->unit = rv_module_graph_at(&checker->program->graph, index);
        if (!collect_locals(checker)) {
            return false;
        }
    }

    for (size_t index = 0; index < checker->program->graph.modules.length; ++index) {
        checker->unit = rv_module_graph_at(&checker->program->graph, index);
        checker->scope = checker->unit->scope;
        if (!bind_imports(checker)) {
            return false;
        }
    }

    for (size_t index = 0; index < checker->program->graph.modules.length; ++index) {
        checker->unit = rv_module_graph_at(&checker->program->graph, index);
        checker->scope = checker->unit->scope;
        for (RvAstDecl *declaration = checker->unit->ast.declarations;
             declaration != NULL; declaration = declaration->next) {
            bool success;
            if (declaration->kind == RV_AST_OBJECT) {
                success = record_fields(checker, declaration);
            } else if (declaration->kind == RV_AST_FUNCTION) {
                success = function_signature(checker, declaration);
            } else {
                success = constant_type(checker, declaration);
            }
            if (!success) {
                return false;
            }
        }
    }
    return true;
}

static bool check_function(RvChecker *checker, RvAstDecl *declaration) {
    RvSymbol *function = declaration->symbol;
    RvScope *scope = rv_check_scope(checker, checker->unit->scope, declaration->span);
    if (scope == NULL) {
        return false;
    }
    checker->scope = scope;
    checker->result = function->type;
    RvAstMember *member = declaration->as.function.parameters;

    for (RvParameter *parameter = function->parameters; parameter != NULL; parameter = parameter->next) {
        RvSymbol *binding = rv_check_symbol(checker, RV_SYMBOL_PARAMETER, parameter->name);
        if (binding == NULL || !rv_check_define(checker, binding)) {
            return false;
        }
        binding->type = parameter->type;
        parameter->binding = binding;
        member->symbol = binding;
        member = member->next;
    }

    bool returns;
    if (!rv_check_block(checker, declaration->as.function.body, scope, &returns)) {
        return false;
    }
    if (checker->result.kind != RV_TYPE_VOID && !returns) {
        return rv_check_fail(checker, RV_CHECK_RETURN, declaration->span,
                             "non-void function does not return on every path");
    }
    return true;
}

bool rv_check_bodies(RvChecker *checker) {
    for (size_t index = 0; index < checker->program->graph.modules.length; ++index) {
        checker->unit = rv_module_graph_at(&checker->program->graph, index);
        for (RvAstDecl *declaration = checker->unit->ast.declarations;
             declaration != NULL; declaration = declaration->next) {
            if (declaration->kind == RV_AST_FUNCTION && !check_function(checker, declaration)) {
                return false;
            }
        }
        RvAstDecl *launch = checker->unit->ast.launch;
        if (launch != NULL) {
            launch->symbol = rv_check_symbol(checker, RV_SYMBOL_FUNCTION, launch->name);
            if (launch->symbol == NULL) {
                return false;
            }
            launch->symbol->type.kind = RV_TYPE_VOID;
            launch->symbol->declaration = launch;
            if (!check_function(checker, launch)) {
                return false;
            }
        }
    }
    return true;
}
