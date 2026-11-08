#include "output.h"
#include "temporary.h"
#include "compiler/common/host/win32/utf8.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static bool valid_destination(const wchar_t *path) {
    DWORD attributes = GetFileAttributesW(path);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
            return true;
        }
        rv_windows_error(error);
        return false;
    }
    if (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE)) {
        errno = EINVAL;
        return false;
    }
    return true;
}

bool rv_output_open(RvOutput *output, const char *destination) {
    *output = (RvOutput){.destination = destination};
    wchar_t *wide = rv_windows_path(destination);
    if (wide == NULL) {
        return false;
    }
    bool valid = valid_destination(wide);
    free(wide);
    if (!valid) {
        return false;
    }
    const char *slash = strrchr(destination, '/');
    const char *backslash = strrchr(destination, '\\');
    if (backslash != NULL && (slash == NULL || backslash > slash)) {
        slash = backslash;
    }
    size_t length = slash == NULL ? 0 : (size_t)(slash - destination) + 1;
    char *parent = malloc(length + 2);
    if (parent == NULL) {
        return false;
    }
    memcpy(parent, destination, length);
    if (length == 0) {
        parent[length++] = '.';
    }
    parent[length] = '\0';
    bool created = rv_windows_temporary(parent, ".ruva-output-", ".exe", false,
                                        &output->temporary, &output->stream);
    free(parent);
    output->created = created;
    return created;
}

bool rv_output_close(RvOutput *output) {
    if (output->stream == NULL) {
        return true;
    }
    FILE *stream = output->stream;
    output->stream = NULL;
    return fclose(stream) == 0;
}

bool rv_output_publish(RvOutput *output) {
    if (!rv_output_close(output)) {
        return false;
    }
    wchar_t *source = rv_windows_path(output->temporary);
    wchar_t *destination = rv_windows_path(output->destination);
    bool success = source != NULL && destination != NULL && valid_destination(destination)
        && MoveFileExW(source, destination, MOVEFILE_REPLACE_EXISTING);
    if (!success) {
        rv_windows_error(GetLastError());
    }
    free(source);
    free(destination);
    if (success) {
        output->created = false;
    }
    return success;
}

void rv_output_discard(RvOutput *output) {
    rv_output_close(output);
    if (output->temporary != NULL) {
        if (output->created) {
            wchar_t *wide = rv_windows_path(output->temporary);
            if (wide != NULL) {
                DeleteFileW(wide);
            }
            free(wide);
        }
        free(output->temporary);
        output->temporary = NULL;
    }
}
