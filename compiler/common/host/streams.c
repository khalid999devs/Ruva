#include "streams.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>
#endif

bool rv_host_initialize_streams(void) {
#ifdef _WIN32
    if (_setmode(_fileno(stdin), _O_BINARY) == -1
        || _setmode(_fileno(stdout), _O_BINARY) == -1
        || _setmode(_fileno(stderr), _O_BINARY) == -1) {
        return false;
    }
    DWORD mode;
    if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) && !SetConsoleCP(CP_UTF8)) {
        return false;
    }
    bool console_output = GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode)
        || GetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), &mode);
    if (console_output && !SetConsoleOutputCP(CP_UTF8)) {
        return false;
    }
#endif
    return true;
}
