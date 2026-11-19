#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

int wmain(int argc, wchar_t **argv) {
    FILE *record = fopen("arguments.bin", "wb");
    if (record == NULL) {
        return 90;
    }
    const wchar_t *output = NULL;
    for (int index = 1; index < argc; ++index) {
        int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argv[index], -1,
                                        NULL, 0, NULL, NULL);
        char *value = count <= 0 ? NULL : malloc((size_t)count);
        if (value == NULL) {
            fclose(record);
            return 91;
        }
        if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argv[index], -1,
                                value, count, NULL, NULL) != count
            || fwrite(value, 1, (size_t)count, record) != (size_t)count) {
            free(value);
            fclose(record);
            return 92;
        }
        free(value);
        if (wcscmp(argv[index], L"-o") == 0 && index + 1 < argc) {
            output = argv[index + 1];
        }
    }
    if (fclose(record) != 0) {
        return 93;
    }
    const char *mode = getenv("RV_MOCK_MODE");
    if (mode != NULL && strcmp(mode, "empty") == 0) {
        return 0;
    }
    if (mode != NULL && strcmp(mode, "invalid") == 0 && output != NULL) {
        FILE *stream = _wfopen(output, L"wb");
        if (stream == NULL) {
            return 94;
        }
        fputs("not an executable", stream);
        return fclose(stream) == 0 ? 0 : 95;
    }
    return 7;
}
