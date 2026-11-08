#ifndef RUVA_WINDOWS_TEMPORARY_H
#define RUVA_WINDOWS_TEMPORARY_H

#include <stdbool.h>
#include <stdio.h>

bool rv_windows_temporary(const char *parent, const char *prefix, const char *suffix,
                           bool directory, char **path, FILE **stream);

#endif
