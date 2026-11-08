#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE

#include "host.h"
#include "compiler/common/memory/allocator.h"

#include <errno.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

char *rv_toolchain_environment(const char *name) {
    errno = 0;
    const char *value = getenv(name);
    return value == NULL ? NULL : strdup(value);
}

char *rv_toolchain_executable(const char *invocation) {
    if (strchr(invocation, '/') != NULL) {
        return realpath(invocation, NULL);
    }
    const char *environment = getenv("PATH");
    char *paths = strdup(environment == NULL ? "/usr/bin:/bin" : environment);
    if (paths == NULL) {
        return NULL;
    }
    char *part = paths;
    char *resolved = NULL;
    while (part != NULL) {
        char *end = strchr(part, ':');
        if (end != NULL) {
            *end = '\0';
        }
        const char *directory = part[0] == '\0' ? "." : part;
        size_t length;
        char *candidate = NULL;
        if (rv_size_add(strlen(directory), strlen(invocation), &length)
            && rv_size_add(length, 2, &length)) {
            candidate = malloc(length);
        }
        if (candidate != NULL) {
            snprintf(candidate, length, "%s/%s", directory, invocation);
            if (access(candidate, X_OK) == 0) {
                resolved = realpath(candidate, NULL);
            }
        }
        free(candidate);
        if (resolved != NULL || end == NULL) {
            break;
        }
        part = end + 1;
    }
    free(paths);
    return resolved;
}

char *rv_toolchain_workspace(void) {
    const char *temporary_root = getenv("TMPDIR");
    char *root = realpath(temporary_root == NULL ? "/tmp" : temporary_root, NULL);
    if (root == NULL) {
        return NULL;
    }
    size_t size;
    char *directory = NULL;
    if (rv_size_add(strlen(root), sizeof("/ruvac-XXXXXX"), &size)) {
        directory = malloc(size);
    }
    if (directory != NULL) {
        snprintf(directory, size, "%s/ruvac-XXXXXX", root);
        if (mkdtemp(directory) == NULL) {
            free(directory);
            directory = NULL;
        }
    }
    free(root);
    return directory;
}

bool rv_toolchain_run(const char *compiler, char *const *arguments, bool *started) {
    pid_t child;
    int result = posix_spawnp(&child, compiler, NULL, NULL, arguments, environ);
    *started = result == 0;
    if (!*started) {
        errno = result;
        return false;
    }
    int status;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    return waited >= 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool rv_toolchain_executable_file(const char *path) {
    struct stat info;
    return stat(path, &info) == 0 && S_ISREG(info.st_mode)
        && info.st_size > 0 && (info.st_mode & S_IXUSR) != 0;
}

void rv_toolchain_remove_file(const char *path) {
    unlink(path);
}

void rv_toolchain_remove_directory(const char *path) {
    rmdir(path);
}
