#include "temporary.h"
#include "compiler/common/host/win32/utf8.h"
#include "compiler/common/memory/allocator.h"

#include <bcrypt.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

bool rv_windows_temporary(const char *parent, const char *prefix, const char *suffix,
                           bool directory, char **path, FILE **stream) {
    *path = NULL;
    if (stream != NULL) {
        *stream = NULL;
    }
    size_t capacity;
    if (!rv_size_add(strlen(parent), strlen(prefix), &capacity)
        || !rv_size_add(capacity, strlen(suffix), &capacity)
        || !rv_size_add(capacity, 34, &capacity)) {
        errno = ENOMEM;
        return false;
    }
    char *candidate = malloc(capacity);
    if (candidate == NULL) {
        return false;
    }
    for (size_t attempt = 0; attempt < 16; ++attempt) {
        unsigned char random[16];
        if (BCryptGenRandom(NULL, random, sizeof(random), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
            errno = EIO;
            break;
        }
        size_t position = (size_t)snprintf(candidate, capacity, "%s/%s", parent, prefix);
        const char digits[] = "0123456789abcdef";
        for (size_t index = 0; index < sizeof(random); ++index) {
            candidate[position++] = digits[random[index] >> 4];
            candidate[position++] = digits[random[index] & 15];
        }
        memcpy(candidate + position, suffix, strlen(suffix) + 1);
        wchar_t *wide = rv_windows_path(candidate);
        if (wide == NULL) {
            break;
        }
        if (directory) {
            bool created = CreateDirectoryW(wide, NULL) != 0;
            DWORD error = GetLastError();
            free(wide);
            if (created) {
                *path = candidate;
                return true;
            }
            rv_windows_error(error);
        } else {
            HANDLE handle = CreateFileW(wide, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                        CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, NULL);
            DWORD error = GetLastError();
            if (handle != INVALID_HANDLE_VALUE) {
                int descriptor = _open_osfhandle((intptr_t)handle, _O_BINARY | _O_RDWR);
                FILE *opened = descriptor < 0 ? NULL : _fdopen(descriptor, "w+b");
                if (opened != NULL) {
                    free(wide);
                    *path = candidate;
                    *stream = opened;
                    return true;
                }
                if (descriptor >= 0) {
                    _close(descriptor);
                } else {
                    CloseHandle(handle);
                }
                DeleteFileW(wide);
                free(wide);
                break;
            }
            free(wide);
            rv_windows_error(error);
        }
        if (errno != EEXIST) {
            break;
        }
    }
    free(candidate);
    return false;
}
