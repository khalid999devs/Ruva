#ifndef RUVA_FILE_INTERNAL_H
#define RUVA_FILE_INTERNAL_H

#include "../ruva_runtime.h"

typedef enum {
    RV_FILE_READ,
    RV_FILE_WRITE,
    RV_FILE_APPEND
} RvFileMode;

struct RvFile {
    FILE *stream;
    RvFileMode mode;
    RvLocation location;
    RvFile *next;
};

RvFile *rv_file_create(RvRuntime *runtime, const RvText *path,
                       RvFileMode mode, RvLocation where);

#endif
