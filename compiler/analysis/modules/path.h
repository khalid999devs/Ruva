#ifndef RUVA_MODULE_PATH_H
#define RUVA_MODULE_PATH_H

#include "module.h"

char *rv_module_canonical_path(RvArena *storage, const char *path, int *system_error);
char *rv_module_import_path(RvArena *storage, const char *importer, const char *relative);
bool rv_module_relative_path(const char *path);

#endif
