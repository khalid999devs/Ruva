#ifndef RUVA_SCANNER_H
#define RUVA_SCANNER_H

#include "lexer.h"

#include <setjmp.h>

typedef struct {
    const RvSource *source;
    RvTokenStream *output;
    RvArena scratch;
    RvLexError error;
    size_t offset;
    size_t start;
    jmp_buf recovery;
} RvScanState;

int rv_scan_emit(RvScanState *state, RvTokenKind kind, const char *bytes, size_t length);
int rv_scan_error(RvScanState *state, RvLexErrorKind kind, size_t start,
                  size_t end, const char *message);
int rv_scan_unterminated_text(RvScanState *state);
_Noreturn void rv_scan_fatal(RvScanState *state);
bool rv_scan_run(RvScanState *state);

#endif
