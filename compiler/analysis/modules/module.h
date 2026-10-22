#ifndef RUVA_MODULE_H
#define RUVA_MODULE_H

#include "compiler/analysis/diagnostics/error.h"
#include "compiler/frontend/ast/ast.h"
#include "compiler/common/memory/vector.h"

typedef struct RvScope RvScope;
typedef struct RvModuleUnit RvModuleUnit;

typedef struct RvImportEdge {
    const RvAstImport *syntax;
    RvModuleUnit *target;
    struct RvImportEdge *next;
} RvImportEdge;

struct RvModuleUnit {
    size_t id;
    const char *path;
    RvSource *source;
    RvAstModule ast;
    RvScope *scope;
    RvImportEdge *imports;
    unsigned int state;
    size_t token_count;
};

typedef struct {
    RvArena storage;
    RvVector modules;
    RvModuleUnit *entry;
    const char *input_path;
} RvModuleGraph;

/* Failed graphs retain sources for diagnostics until explicitly destroyed. */
bool rv_module_graph_load(const char *path, RvAllocator allocator,
                           RvModuleGraph *output, RvCheckError *error);
void rv_module_graph_destroy(RvModuleGraph *graph);
RvModuleUnit *rv_module_graph_at(const RvModuleGraph *graph, size_t index);

#endif
