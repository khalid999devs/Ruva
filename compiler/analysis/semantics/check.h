#ifndef RUVA_CHECK_H
#define RUVA_CHECK_H

#include "compiler/analysis/modules/module.h"
#include "compiler/analysis/symbols/symbol.h"

typedef struct {
    RvModuleGraph graph;
    RvArena storage;
    RvVector symbols;
    bool valid;
} RvCheckedProgram;

/* Output must be new/destroyed. Keep it alive while using error spans. */
bool rv_check_program(const char *path, RvAllocator allocator,
                       RvCheckedProgram *output, RvCheckError *error);
void rv_checked_program_destroy(RvCheckedProgram *program);
RvSymbol *rv_checked_symbol_at(const RvCheckedProgram *program, size_t index);

#endif
