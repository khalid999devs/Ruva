#define _XOPEN_SOURCE 700

#include "files.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "win32/utf8.h"

static HANDLE open_path(const char *path) {
    wchar_t *wide = rv_windows_path(path);
    if (wide == NULL) {
        return INVALID_HANDLE_VALUE;
    }
    HANDLE handle = CreateFileW(wide, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    DWORD error = GetLastError();
    free(wide);
    if (handle == INVALID_HANDLE_VALUE) {
        rv_windows_error(error);
    }
    return handle;
}

FILE *rv_host_fopen(const char *path, const char *mode) {
    wchar_t *wide = rv_windows_path(path);
    wchar_t *wide_mode = rv_windows_wide(mode);
    FILE *stream = NULL;
    if (wide != NULL && wide_mode != NULL) {
        stream = _wfopen(wide, wide_mode);
    }
    free(wide);
    free(wide_mode);
    return stream;
}

char *rv_host_canonical_path(const char *path) {
    HANDLE handle = open_path(path);
    if (handle == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    DWORD count = GetFinalPathNameByHandleW(handle, NULL, 0, FILE_NAME_NORMALIZED);
    wchar_t *wide = count == 0 ? NULL : calloc((size_t)count + 1, sizeof(*wide));
    char *result = NULL;
    if (wide != NULL) {
        DWORD written = GetFinalPathNameByHandleW(handle, wide, count + 1, FILE_NAME_NORMALIZED);
        if (written != 0 && written <= count) {
            if (wcsncmp(wide, L"\\\\?\\UNC\\", 8) == 0) {
                wide[6] = L'\\';
                result = rv_windows_utf8(wide + 6);
            } else {
                const wchar_t *start = wcsncmp(wide, L"\\\\?\\", 4) == 0 ? wide + 4 : wide;
                result = rv_windows_utf8(start);
            }
        } else {
            rv_windows_error(GetLastError());
        }
    } else if (count == 0) {
        rv_windows_error(GetLastError());
    }
    free(wide);
    CloseHandle(handle);
    if (result != NULL) {
        for (char *part = result; *part != '\0'; ++part) {
            if (*part == '\\') {
                *part = '/';
            }
        }
    }
    return result;
}

bool rv_host_regular_file(const char *path) {
    HANDLE handle = open_path(path);
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    BY_HANDLE_FILE_INFORMATION information;
    bool regular = GetFileType(handle) == FILE_TYPE_DISK
        && GetFileInformationByHandle(handle, &information)
        && !(information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    CloseHandle(handle);
    return regular;
}

bool rv_host_same_file(const char *left, const char *right) {
    HANDLE first = open_path(left);
    HANDLE second = open_path(right);
    BY_HANDLE_FILE_INFORMATION first_info;
    BY_HANDLE_FILE_INFORMATION second_info;
    bool same = first != INVALID_HANDLE_VALUE && second != INVALID_HANDLE_VALUE
        && GetFileInformationByHandle(first, &first_info)
        && GetFileInformationByHandle(second, &second_info)
        && first_info.dwVolumeSerialNumber == second_info.dwVolumeSerialNumber
        && first_info.nFileIndexHigh == second_info.nFileIndexHigh
        && first_info.nFileIndexLow == second_info.nFileIndexLow;
    if (first != INVALID_HANDLE_VALUE) {
        CloseHandle(first);
    }
    if (second != INVALID_HANDLE_VALUE) {
        CloseHandle(second);
    }
    return same;
}

#else
#include <sys/stat.h>

FILE *rv_host_fopen(const char *path, const char *mode) {
    return fopen(path, mode);
}

char *rv_host_canonical_path(const char *path) {
    return realpath(path, NULL);
}

bool rv_host_regular_file(const char *path) {
    struct stat info;
    return stat(path, &info) == 0 && S_ISREG(info.st_mode);
}

bool rv_host_same_file(const char *left, const char *right) {
    struct stat first;
    struct stat second;
    return stat(left, &first) == 0 && stat(right, &second) == 0
        && first.st_dev == second.st_dev && first.st_ino == second.st_ino;
}
#endif
