#define _XOPEN_SOURCE 700

#include "path.h"
#include "compiler/common/host/files.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

char *rv_module_canonical_path(RvArena *storage, const char *path, int *system_error) {
    errno = 0;
    char *resolved = rv_host_canonical_path(path);
    if (resolved == NULL) {
        *system_error = errno;
        return NULL;
    }

    char *copy = rv_arena_copy(storage, resolved, strlen(resolved));
    free(resolved);
    *system_error = copy == NULL ? ENOMEM : 0;
    return copy;
}

bool rv_module_relative_path(const char *path) {
    if (path[0] == '\0' || path[0] == '/' || path[0] == '\\') {
        return false;
    }

    bool letter = (path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z');
    return !letter || path[1] != ':';
}

char *rv_module_import_path(RvArena *storage, const char *importer, const char *relative) {
    const char *slash = strrchr(importer, '/');
    size_t directory = slash == NULL ? 0 : (size_t)(slash - importer) + 1;
    size_t tail = strlen(relative);
    size_t size;
    if (!rv_size_add(directory, tail, &size) || !rv_size_add(size, 1, &size)) {
        return NULL;
    }

    char *path = rv_arena_allocate(storage, size);
    if (path != NULL) {
        memcpy(path, importer, directory);
        memcpy(path + directory, relative, tail + 1);
    }
    return path;
}
