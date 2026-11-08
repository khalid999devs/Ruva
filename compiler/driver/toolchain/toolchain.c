#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE

#include "toolchain.h"
#include "host.h"
#include "compiler/backend/c/emit.h"
#include "compiler/common/diagnostics/diagnostic.h"
#include "compiler/common/host/files.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static char *join(const char *left, const char *right) {
    size_t size;
    if (!rv_size_add(strlen(left), strlen(right), &size) || !rv_size_add(size, 2, &size)) {
        return NULL;
    }
    char *path = malloc(size);
    if (path != NULL) {
        snprintf(path, size, "%s/%s", left, right);
    }
    return path;
}

static bool runtime_directory(const char *directory) {
    char *header = join(directory, "ruva_runtime.h");
    bool exists = header != NULL && rv_host_regular_file(header);
    free(header);
    return exists;
}

static char *find_runtime(const char *invocation) {
    char *override = rv_toolchain_environment("RUVA_RUNTIME_ROOT");
    if (override == NULL && errno != 0) {
        return NULL;
    }
    if (override != NULL) {
        char *resolved = rv_host_canonical_path(override);
        free(override);
        if (resolved != NULL && runtime_directory(resolved)) {
            return resolved;
        }
        free(resolved);
        return NULL;
    }
    char *path = rv_toolchain_executable(invocation);
    while (path != NULL) {
        char *slash = strrchr(path, '/');
        if (slash == NULL) {
            break;
        }
        *slash = '\0';
        char *candidate = join(path[0] == '\0' ? "/" : path, "runtime");
        if (candidate != NULL && runtime_directory(candidate)) {
            free(path);
            return candidate;
        }
        free(candidate);
        if (slash == path) {
            break;
        }
    }
    free(path);
    return NULL;
}

static bool run_compiler(const char *source, const char *output, const char *runtime, bool sanitize) {
    const char *sources[] = {
        "core/runtime.c", "core/memory.c", "core/numeric.c",
        "text/text.c", "text/format.c", "text/output.c",
        "text/snapshot.c", "collections/array.c",
        "conversion/parse.c", "conversion/numeric.c", "io/read.c", "io/input.c",
        "files/handle.c", "files/path.c",
        "../compiler/common/host/files.c", "../compiler/common/host/numeric.c",
        "../compiler/common/host/streams.c",
#ifdef _WIN32
        "../compiler/common/host/win32/utf8.c",
#endif
    };
    enum { source_count = sizeof(sources) / sizeof(sources[0]) };
    char *paths[source_count] = {0};
    bool success = false;
    char *compiler = rv_toolchain_environment("CC");
    if (compiler == NULL && errno != 0) {
        fputs("ruvac: error: cannot read the C compiler setting\n", stderr);
        goto cleanup;
    }
    for (size_t index = 0; index < source_count; ++index) {
        paths[index] = join(runtime, sources[index]);
        if (paths[index] == NULL) {
            fputs("ruvac: error: out of memory preparing C compiler\n", stderr);
            goto cleanup;
        }
    }
    char *arguments[48];
    size_t count = 0;
    arguments[count++] = compiler;
    if (compiler == NULL) {
#ifdef _WIN32
        arguments[0] = "gcc";
#else
        arguments[0] = "cc";
#endif
    }
    arguments[count++] = "-std=c11";
    arguments[count++] = "-Wall";
    arguments[count++] = "-Wextra";
    arguments[count++] = "-Wpedantic";
    arguments[count++] = "-Werror";
    arguments[count++] = "-fno-fast-math";
    arguments[count++] = "-ffp-contract=off";
    if (sanitize) {
        arguments[count++] = "-fsanitize=address,undefined";
        arguments[count++] = "-fno-omit-frame-pointer";
        arguments[count++] = "-fno-sanitize-recover=all";
    }
    arguments[count++] = "-I";
    arguments[count++] = (char *)runtime;
    arguments[count++] = (char *)source;
    for (size_t index = 0; index < source_count; ++index) {
        arguments[count++] = paths[index];
    }
    arguments[count++] = "-o";
    arguments[count++] = (char *)output;
    arguments[count] = NULL;
    bool started;
    if (!rv_toolchain_run(arguments[0], arguments, &started)) {
        if (!started) {
            fprintf(stderr, "ruvac: error: cannot start C compiler: %s\n", strerror(errno));
        } else {
            fputs("ruvac: error: C compiler failed\n", stderr);
        }
        goto cleanup;
    }
    success = rv_toolchain_executable_file(output);
    if (!success) {
        fputs("ruvac: error: C compiler did not produce an executable\n", stderr);
    }

cleanup:
    free(compiler);
    for (size_t index = 0; index < source_count; ++index) {
        free(paths[index]);
    }
    return success;
}

bool rv_toolchain_build(const RvCheckedProgram *program, const char *output,
                         const char *invocation, bool sanitize) {
#ifdef _WIN32
    if (sanitize) {
        fputs("ruvac: error: --sanitize is unavailable on the Windows GCC target\n", stderr);
        return false;
    }
#endif
    char *runtime = find_runtime(invocation);
    char *directory = NULL;
    char *source = NULL;
    bool success = false;
    if (runtime == NULL) {
        fputs("ruvac: error: cannot locate runtime sources\n", stderr);
        goto cleanup;
    }
    directory = rv_toolchain_workspace();
    if (directory == NULL) {
        fputs("ruvac: error: cannot create compiler workspace\n", stderr);
        goto cleanup;
    }
    source = join(directory, "program.c");
    FILE *stream = source == NULL ? NULL : rv_host_fopen(source, "wb");
    if (stream == NULL) {
        fputs("ruvac: error: cannot create generated C\n", stderr);
        goto cleanup;
    }
    RvEmitError error;
    success = rv_c_emit(stream, program, rv_allocator_system(), &error);
    if (fclose(stream) != 0) {
        success = false;
    }
    if (!success) {
        if (error.span.source != NULL) {
            rv_diagnostic_write(stderr, error.span, error.message);
        } else {
            fputs("ruvac: error: cannot emit generated C\n", stderr);
        }
        goto cleanup;
    }
    success = run_compiler(source, output, runtime, sanitize);

cleanup:
    if (source != NULL) {
        rv_toolchain_remove_file(source);
    }
    if (directory != NULL) {
        rv_toolchain_remove_directory(directory);
    }
    free(source);
    free(directory);
    free(runtime);
    return success;
}
