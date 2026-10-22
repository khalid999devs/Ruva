#ifndef RUVA_CHECK_ERROR_H
#define RUVA_CHECK_ERROR_H

#include "compiler/common/source/span.h"

#include <stdio.h>

typedef enum {
    RV_CHECK_OK,
    RV_CHECK_ARGUMENT,
    RV_CHECK_IO,
    RV_CHECK_LEXICAL,
    RV_CHECK_SYNTAX,
    RV_CHECK_IMPORT,
    RV_CHECK_NAME,
    RV_CHECK_TYPE,
    RV_CHECK_OWNERSHIP,
    RV_CHECK_RETURN,
    RV_CHECK_ALLOCATION,
    RV_CHECK_LIMIT
} RvCheckErrorKind;

typedef struct {
    RvCheckErrorKind kind;
    RvSpan span;
    const char *path;
    const char *message;
    int system_error;
} RvCheckError;

bool rv_check_error_write(FILE *output, const RvCheckError *error);

#endif
