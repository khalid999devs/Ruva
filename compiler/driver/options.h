#ifndef RUVA_OPTIONS_H
#define RUVA_OPTIONS_H

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    const char *source;
    const char *output;
    bool check;
    bool emit_c;
    bool sanitize;
    bool help;
} RvOptions;

bool rv_options_parse(int argc, char **argv, RvOptions *options, const char **error);
void rv_options_usage(FILE *output);

#endif
