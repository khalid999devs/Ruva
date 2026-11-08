#ifndef RUVA_OUTPUT_H
#define RUVA_OUTPUT_H

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    const char *destination;
    char *temporary;
    FILE *stream;
    bool created;
} RvOutput;

bool rv_output_open(RvOutput *output, const char *destination);
bool rv_output_close(RvOutput *output);
bool rv_output_publish(RvOutput *output);
void rv_output_discard(RvOutput *output);

#endif
