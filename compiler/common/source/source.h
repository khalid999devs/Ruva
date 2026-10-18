#ifndef RUVA_SOURCE_H
#define RUVA_SOURCE_H

#include "compiler/common/memory/allocator.h"

typedef struct RvSource RvSource;

typedef enum {
    RV_SOURCE_OK,
    RV_SOURCE_INVALID,
    RV_SOURCE_IO,
    RV_SOURCE_ALLOCATION,
    RV_SOURCE_TOO_LARGE
} RvSourceErrorKind;

typedef struct {
    RvSourceErrorKind kind;
    int system_error;
} RvSourceError;

typedef struct {
    size_t line;
    size_t column;
} RvSourcePosition;

typedef struct {
    const unsigned char *bytes;
    size_t length;
} RvSourceLine;

RvSource *rv_source_from_bytes(RvAllocator allocator, const char *path,
                               const void *bytes, size_t length, RvSourceError *error);
RvSource *rv_source_load(RvAllocator allocator, const char *path, RvSourceError *error);
void rv_source_destroy(RvSource *source);
const char *rv_source_path(const RvSource *source);
const unsigned char *rv_source_bytes(const RvSource *source);
size_t rv_source_length(const RvSource *source);
size_t rv_source_line_count(const RvSource *source);
/* Positions are one-based byte columns; offset == length locates EOF. */
bool rv_source_position(const RvSource *source, size_t offset, RvSourcePosition *position);
bool rv_source_line(const RvSource *source, size_t line, RvSourceLine *result);

#endif
