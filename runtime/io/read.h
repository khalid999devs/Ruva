#ifndef RUVA_READ_H
#define RUVA_READ_H

#include "../ruva_runtime.h"

RvText *rv_read_text(RvRuntime *runtime, FILE *stream, bool line, RvLocation where);

#endif
