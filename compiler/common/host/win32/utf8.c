#include "utf8.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

void rv_windows_error(DWORD error) {
    switch (error) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            errno = ENOENT;
            break;
        case ERROR_ACCESS_DENIED:
        case ERROR_SHARING_VIOLATION:
            errno = EACCES;
            break;
        case ERROR_ALREADY_EXISTS:
        case ERROR_FILE_EXISTS:
            errno = EEXIST;
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
            errno = ENOMEM;
            break;
        case ERROR_INVALID_NAME:
        case ERROR_INVALID_PARAMETER:
            errno = EINVAL;
            break;
        default:
            errno = EIO;
            break;
    }
}

wchar_t *rv_windows_wide(const char *text) {
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0);
    if (count <= 0) {
        errno = EILSEQ;
        return NULL;
    }
    if ((size_t)count > SIZE_MAX / sizeof(wchar_t)) {
        errno = ENOMEM;
        return NULL;
    }
    wchar_t *result = malloc((size_t)count * sizeof(*result));
    if (result != NULL
        && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, result, count) == 0) {
        free(result);
        errno = EILSEQ;
        return NULL;
    }
    return result;
}

wchar_t *rv_windows_path(const char *path) {
    wchar_t *result = rv_windows_wide(path);
    if (result != NULL) {
        for (wchar_t *part = result; *part != L'\0'; ++part) {
            if (*part == L'/') {
                *part = L'\\';
            }
        }
    }
    return result;
}

char *rv_windows_utf8(const wchar_t *text) {
    int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, NULL, 0, NULL, NULL);
    if (count <= 0) {
        errno = EILSEQ;
        return NULL;
    }
    char *result = malloc((size_t)count);
    if (result != NULL
        && WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1,
                               result, count, NULL, NULL) == 0) {
        free(result);
        errno = EILSEQ;
        return NULL;
    }
    return result;
}
