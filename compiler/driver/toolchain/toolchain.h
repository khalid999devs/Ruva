#ifndef RUVA_TOOLCHAIN_H
#define RUVA_TOOLCHAIN_H

#include "compiler/analysis/semantics/check.h"

bool rv_toolchain_build(const RvCheckedProgram *program, const char *output,
                         const char *invocation, bool sanitize);

#endif
