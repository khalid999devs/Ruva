#ifndef RUVA_C_EMIT_H
#define RUVA_C_EMIT_H

#include "compiler/analysis/semantics/check.h"

typedef enum {
    RV_EMIT_OK,
    RV_EMIT_ARGUMENT,
    RV_EMIT_UNSUPPORTED,
    RV_EMIT_ALLOCATION,
    RV_EMIT_IO,
    RV_EMIT_LIMIT
} RvEmitErrorKind;

typedef struct {
    RvEmitErrorKind kind;
    RvSpan span;
    const char *message;
} RvEmitError;

bool rv_c_supported(const RvCheckedProgram *program, RvEmitError *error);
bool rv_c_emit(FILE *output, const RvCheckedProgram *program,
                RvAllocator allocator, RvEmitError *error);

#endif
