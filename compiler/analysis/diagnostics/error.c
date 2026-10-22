#include "error.h"
#include "compiler/common/diagnostics/diagnostic.h"

bool rv_check_error_write(FILE *output, const RvCheckError *error) {
    if (output == NULL || error == NULL || error->kind == RV_CHECK_OK || error->message == NULL) {
        return false;
    }

    if (error->span.source != NULL) {
        return rv_diagnostic_write(output, error->span, error->message);
    }

    const char *path = error->path == NULL ? "<input>" : error->path;
    return fprintf(output, "%s: error: %s\n", path, error->message) >= 0;
}
