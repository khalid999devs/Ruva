#include "host.h"
#include "../files/temporary.h"
#include "compiler/common/host/files.h"
#include "compiler/common/host/win32/utf8.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

char *rv_toolchain_environment(const char *name) {
    errno = 0;
    wchar_t *wide_name = rv_windows_wide(name);
    if (wide_name == NULL) {
        return NULL;
    }
    const wchar_t *value = _wgetenv(wide_name);
    char *result = value == NULL ? NULL : rv_windows_utf8(value);
    free(wide_name);
    return result;
}

char *rv_toolchain_executable(const char *invocation) {
    (void)invocation;
    wchar_t *wide = calloc(32768, sizeof(*wide));
    if (wide == NULL) {
        return NULL;
    }
    DWORD length = GetModuleFileNameW(NULL, wide, 32768);
    char *path = NULL;
    if (length != 0 && length < 32768) {
        path = rv_windows_utf8(wide);
    }
    free(wide);
    char *canonical = path == NULL ? NULL : rv_host_canonical_path(path);
    free(path);
    return canonical;
}

char *rv_toolchain_workspace(void) {
    char *root = rv_toolchain_environment("TMPDIR");
    if (root == NULL && errno != 0) {
        return NULL;
    }
    if (root == NULL) {
        wchar_t *wide = calloc(32768, sizeof(*wide));
        if (wide == NULL) {
            return NULL;
        }
        DWORD length = GetTempPathW(32768, wide);
        if (length != 0 && length < 32768) {
            root = rv_windows_utf8(wide);
        }
        free(wide);
    }
    if (root == NULL) {
        return NULL;
    }
    char *canonical = rv_host_canonical_path(root);
    free(root);
    if (canonical == NULL) {
        return NULL;
    }
    char *directory = NULL;
    rv_windows_temporary(canonical, "ruvac-", "", true, &directory, NULL);
    free(canonical);
    return directory;
}

static wchar_t *find_compiler(const char *compiler) {
    wchar_t *name = rv_windows_path(compiler);
    if (name == NULL) {
        return NULL;
    }
    DWORD length = SearchPathW(NULL, name, L".exe", 0, NULL, NULL);
    wchar_t *path = length == 0 ? NULL : calloc((size_t)length + 1, sizeof(*path));
    if (path != NULL) {
        DWORD written = SearchPathW(NULL, name, L".exe", length + 1, path, NULL);
        if (written == 0 || written > length) {
            rv_windows_error(GetLastError());
            free(path);
            path = NULL;
        }
    } else if (length == 0) {
        rv_windows_error(GetLastError());
    }
    free(name);
    return path;
}

bool rv_toolchain_run(const char *compiler, char *const *arguments, bool *started) {
    *started = false;
    char *quoted = rv_toolchain_command_line(arguments);
    wchar_t *line = quoted == NULL ? NULL : rv_windows_wide(quoted);
    wchar_t *application = find_compiler(compiler);
    free(quoted);
    if (line == NULL || application == NULL || wcslen(line) >= 32767) {
        free(line);
        free(application);
        return false;
    }
    STARTUPINFOW startup = {.cb = sizeof(startup), .dwFlags = STARTF_USESTDHANDLES};
    PROCESS_INFORMATION process = {0};
    HANDLE handles[3] = {NULL, NULL, NULL};
    DWORD streams[] = {STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE};
    bool success = false;
    for (size_t index = 0; index < 3; ++index) {
        if (!DuplicateHandle(GetCurrentProcess(), GetStdHandle(streams[index]),
                              GetCurrentProcess(), &handles[index], 0, TRUE, DUPLICATE_SAME_ACCESS)) {
            rv_windows_error(GetLastError());
            goto cleanup;
        }
    }
    startup.hStdInput = handles[0];
    startup.hStdOutput = handles[1];
    startup.hStdError = handles[2];
    if (!CreateProcessW(application, line, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &process)) {
        rv_windows_error(GetLastError());
        goto cleanup;
    }
    *started = true;
    DWORD exit_code;
    success = WaitForSingleObject(process.hProcess, INFINITE) == WAIT_OBJECT_0
        && GetExitCodeProcess(process.hProcess, &exit_code) && exit_code == 0;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

cleanup:
    for (size_t index = 0; index < 3; ++index) {
        if (handles[index] != NULL) {
            CloseHandle(handles[index]);
        }
    }
    free(line);
    free(application);
    return success;
}

bool rv_toolchain_executable_file(const char *path) {
    if (!rv_host_regular_file(path)) {
        return false;
    }
    FILE *stream = rv_host_fopen(path, "rb");
    if (stream == NULL) {
        return false;
    }
    unsigned char header[64];
    bool valid = fread(header, 1, sizeof(header), stream) == sizeof(header)
        && header[0] == 'M' && header[1] == 'Z';
    if (valid) {
        uint32_t offset = (uint32_t)header[60] | (uint32_t)header[61] << 8
            | (uint32_t)header[62] << 16 | (uint32_t)header[63] << 24;
        unsigned char signature[24];
        valid = _fseeki64(stream, offset, SEEK_SET) == 0
            && fread(signature, 1, sizeof(signature), stream) == sizeof(signature)
            && memcmp(signature, "PE\0\0", 4) == 0
            && (signature[22] & 2) != 0 && (signature[23] & 32) == 0;
    }
    if (fclose(stream) != 0) {
        valid = false;
    }
    return valid;
}

void rv_toolchain_remove_file(const char *path) {
    wchar_t *wide = rv_windows_path(path);
    if (wide != NULL) {
        DeleteFileW(wide);
    }
    free(wide);
}

void rv_toolchain_remove_directory(const char *path) {
    wchar_t *wide = rv_windows_path(path);
    if (wide != NULL) {
        RemoveDirectoryW(wide);
    }
    free(wide);
}
