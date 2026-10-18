#ifndef RUVA_SPAN_H
#define RUVA_SPAN_H

#include "source.h"

/* Spans borrow their source and cover [start, end). */
typedef struct {
    const RvSource *source;
    size_t start;
    size_t end;
} RvSpan;

bool rv_span_create(RvSpan *span, const RvSource *source, size_t start, size_t end);

#endif
