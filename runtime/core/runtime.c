#include "../ruva_runtime.h"
#include "../../compiler/common/host/streams.h"

#include <stdlib.h>
#include <signal.h>

static void *allocate(void *context, size_t size) {
    (void)context;
    return malloc(size);
}

static void *resize(void *context, void *pointer, size_t size) {
    (void)context;
    return realloc(pointer, size);
}

static void release(void *context, void *pointer) {
    (void)context;
    free(pointer);
}

void rv_runtime_init(RvRuntime *runtime, FILE *output, FILE *errors) {
    *runtime = (RvRuntime){
        .allocator = {NULL, allocate, resize, release},
        .input = stdin,
        .output = output,
        .errors = errors
    };
    if (!rv_host_initialize_streams()) {
        rv_panic(runtime, "cannot configure standard streams", (RvLocation){0});
    }
#ifdef SIGPIPE
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        rv_panic(runtime, "cannot configure output errors", (RvLocation){0});
    }
#endif
}

void rv_report_error(RvRuntime *runtime, const char *message, RvLocation where) {
    /* Preserve the original diagnostic even if flushing output fails. */
    (void)fflush(runtime->output);
    const char *path = where.path == NULL ? "<runtime>" : where.path;
    if (where.line != 0) {
        fprintf(runtime->errors, "%s:%zu:%zu: error: %s\n", path, where.line, where.column, message);
    } else {
        fprintf(runtime->errors, "%s: error: %s\n", path, message);
    }
}

bool rv_runtime_finish(RvRuntime *runtime) {
    bool success = fflush(runtime->output) == 0;
    if (!success) {
        rv_report_error(runtime, "output failed", runtime->output_location);
    }
    if (!rv_runtime_cleanup(runtime)) {
        success = false;
    }
    return success;
}

_Noreturn void rv_panic(RvRuntime *runtime, const char *message, RvLocation where) {
    rv_report_error(runtime, message, where);
    rv_runtime_cleanup(runtime);
    exit(EXIT_FAILURE);
}
