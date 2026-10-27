#ifndef RUVA_TEXT_INTERNAL_H
#define RUVA_TEXT_INTERNAL_H

#include "../ruva_runtime.h"

struct RvText {
    size_t references;
    size_t length;
    char bytes[];
};

#endif
