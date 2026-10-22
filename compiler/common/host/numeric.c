#define _GNU_SOURCE

#include "numeric.h"

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <xlocale.h>
#endif

bool rv_host_parse_real(const char *text, char **end, double *value) {
#ifdef _WIN32
    _locale_t locale = _create_locale(LC_NUMERIC, "C");
#else
    locale_t locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
#endif
    if (locale == NULL) {
        return false;
    }
    errno = 0;
#ifdef _WIN32
    *value = _strtod_l(text, end, locale);
    int parse_error = errno;
    _free_locale(locale);
#else
    *value = strtod_l(text, end, locale);
    int parse_error = errno;
    freelocale(locale);
#endif
    errno = parse_error;
    return true;
}

int rv_host_format_real(char *buffer, size_t capacity, const char *format, double value) {
#ifdef _WIN32
    _locale_t locale = _create_locale(LC_NUMERIC, "C");
    if (locale == NULL) {
        return -1;
    }
    int written = _snprintf_l(buffer, capacity, format, locale, value);
    _free_locale(locale);
#else
    locale_t locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
    if (locale == NULL) {
        return -1;
    }
    locale_t previous = uselocale(locale);
    if (previous == (locale_t)0) {
        freelocale(locale);
        return -1;
    }
    int written = snprintf(buffer, capacity, format, value);
    uselocale(previous);
    freelocale(locale);
#endif
    return written;
}
