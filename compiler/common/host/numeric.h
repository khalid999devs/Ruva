#ifndef RUVA_HOST_NUMERIC_H
#define RUVA_HOST_NUMERIC_H

#include <stdbool.h>
#include <stddef.h>

bool rv_host_parse_real(const char *text, char **end, double *value);
int rv_host_format_real(char *buffer, size_t capacity, const char *format, double value);

#endif
