#ifndef RUVA_TOOLCHAIN_HOST_H
#define RUVA_TOOLCHAIN_HOST_H

#include <stdbool.h>

char *rv_toolchain_environment(const char *name);
char *rv_toolchain_executable(const char *invocation);
char *rv_toolchain_workspace(void);
bool rv_toolchain_run(const char *compiler, char *const *arguments, bool *started);
bool rv_toolchain_executable_file(const char *path);
void rv_toolchain_remove_file(const char *path);
void rv_toolchain_remove_directory(const char *path);
char *rv_toolchain_command_line(char *const *arguments);

#endif
