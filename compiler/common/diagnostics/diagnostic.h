#ifndef RUVA_DIAGNOSTIC_H
#define RUVA_DIAGNOSTIC_H

#include "compiler/common/source/span.h"

#include <stdio.h>

bool rv_diagnostic_write(FILE *stream, RvSpan span, const char *message);
bool rv_diagnostic_load_error(FILE *stream, const char *path, RvSourceError error);

#endif
