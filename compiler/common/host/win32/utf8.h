#ifndef RUVA_WINDOWS_UTF8_H
#define RUVA_WINDOWS_UTF8_H

#include <windows.h>

wchar_t *rv_windows_wide(const char *text);
wchar_t *rv_windows_path(const char *path);
char *rv_windows_utf8(const wchar_t *text);
void rv_windows_error(DWORD error);

#endif
