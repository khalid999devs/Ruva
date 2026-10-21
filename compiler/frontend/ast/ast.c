#include "ast.h"

void rv_ast_module_destroy(RvAstModule *module) {
    rv_arena_destroy(&module->storage);
    module->source = NULL;
    module->span = (RvSpan){0};
    module->imports = NULL;
    module->declarations = NULL;
    module->launch = NULL;
}
