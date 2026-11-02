#include "../internal.h"

static void signature(RvCEmitter *emitter, const RvSymbol *function) {
    rv_c_write(emitter, "static ");
    rv_c_write_type(emitter, function->type);
    rv_c_write(emitter, " rv_f_%zu(RvRuntime *rt", function->id);
    for (RvParameter *parameter = function->parameters; parameter != NULL; parameter = parameter->next) {
        rv_c_write(emitter, ", ");
        rv_c_write_type(emitter, parameter->type);
        rv_c_write(emitter, " rv_v_%zu", parameter->binding->id);
    }
    rv_c_write(emitter, ")");
}

void rv_c_functions(RvCEmitter *emitter, const RvCheckedProgram *program) {
    bool any = false;
    for (size_t index = 0; index < program->graph.modules.length; ++index) {
        RvModuleUnit *unit = rv_module_graph_at(&program->graph, index);
        for (RvAstDecl *declaration = unit->ast.declarations;
             declaration != NULL; declaration = declaration->next) {
            if (declaration->kind == RV_AST_FUNCTION) {
                any = true;
                signature(emitter, declaration->symbol);
                rv_c_write(emitter, ";\n");
            }
        }
    }
    if (!any) {
        return;
    }
    /* Ruva permits recursive functions that do not terminate. */
    rv_c_write(emitter,
        "\n#if defined(__clang__)\n"
        "#pragma clang diagnostic ignored \"-Winfinite-recursion\"\n"
        "#elif defined(__GNUC__) && __GNUC__ >= 12\n"
        "#pragma GCC diagnostic ignored \"-Winfinite-recursion\"\n"
        "#endif\n");
    rv_c_write(emitter, "\n");
    for (size_t index = 0; index < program->graph.modules.length; ++index) {
        RvModuleUnit *unit = rv_module_graph_at(&program->graph, index);
        for (RvAstDecl *declaration = unit->ast.declarations;
             declaration != NULL; declaration = declaration->next) {
            if (declaration->kind == RV_AST_FUNCTION) {
                emitter->function = declaration->symbol;
                signature(emitter, declaration->symbol);
                rv_c_write(emitter, "\n");
                rv_c_block(emitter, declaration->as.function.body);
                rv_c_write(emitter, "\n");
            }
        }
    }
}

void rv_c_function_references(RvCEmitter *emitter, const RvCheckedProgram *program) {
    for (size_t index = 0; index < program->graph.modules.length; ++index) {
        RvModuleUnit *unit = rv_module_graph_at(&program->graph, index);
        for (RvAstDecl *declaration = unit->ast.declarations;
             declaration != NULL; declaration = declaration->next) {
            if (declaration->kind == RV_AST_FUNCTION) {
                rv_c_write(emitter, "    (void)rv_f_%zu;\n", declaration->symbol->id);
            }
        }
    }
}
