#ifndef RUVA_HOST_FILES_H
#define RUVA_HOST_FILES_H

#include <stdbool.h>
#include <stdio.h>

FILE *rv_host_fopen(const char *path, const char *mode);
char *rv_host_canonical_path(const char *path);
bool rv_host_regular_file(const char *path);
bool rv_host_same_file(const char *left, const char *right);

#endif
