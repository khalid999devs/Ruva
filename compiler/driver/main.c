#define _XOPEN_SOURCE 700

#include "options.h"
#include "files/output.h"
#include "toolchain/toolchain.h"
#include "compiler/backend/c/emit.h"
#include "compiler/common/diagnostics/diagnostic.h"
#include "compiler/common/host/files.h"
#include "compiler/common/host/streams.h"

#include <stdlib.h>
#ifdef _WIN32
#include "compiler/common/host/win32/utf8.h"
#endif

static bool overwrites_input(const RvCheckedProgram *program, const char *output) {
    for (size_t index = 0; index < program->graph.modules.length; ++index) {
        RvModuleUnit *unit = rv_module_graph_at(&program->graph, index);
        if (rv_host_same_file(unit->path, output)) {
            return true;
        }
    }
    return false;
}

static void write_emit_error(const RvEmitError *error) {
    if (error->span.source != NULL) {
        rv_diagnostic_write(stderr, error->span, error->message);
    } else {
        fprintf(stderr, "ruvac: error: %s\n", error->message);
    }
}

static int run(int argc, char **argv) {
    if (!rv_host_initialize_streams()) {
        fputs("ruvac: error: cannot configure standard streams\n", stderr);
        return EXIT_FAILURE;
    }
    RvOptions options;
    const char *option_error;
    if (!rv_options_parse(argc, argv, &options, &option_error)) {
        fprintf(stderr, "ruvac: error: %s\n", option_error);
        rv_options_usage(stderr);
        return EXIT_FAILURE;
    }
    if (options.help) {
        rv_options_usage(stdout);
        return EXIT_SUCCESS;
    }
    RvCheckedProgram program;
    RvCheckError check_error;
    bool success = rv_check_program(options.source, rv_allocator_system(), &program, &check_error);
    if (!success) {
        rv_check_error_write(stderr, &check_error);
    } else if (!options.check) {
        RvEmitError emit_error;
        if (!rv_c_supported(&program, &emit_error)) {
            write_emit_error(&emit_error);
            success = false;
        } else if (overwrites_input(&program, options.output)) {
            fputs("ruvac: error: output would overwrite a source file\n", stderr);
            success = false;
        } else {
            RvOutput output;
            success = rv_output_open(&output, options.output);
            if (!success) {
                fputs("ruvac: error: cannot create output file\n", stderr);
            } else {
                if (options.emit_c) {
                    success = rv_c_emit(output.stream, &program, rv_allocator_system(), &emit_error);
                    if (!success) {
                        write_emit_error(&emit_error);
                    }
                } else {
                    success = rv_output_close(&output);
                    if (success) {
                        success = rv_toolchain_build(&program, output.temporary, argv[0], options.sanitize);
                    } else {
                        fputs("ruvac: error: cannot close output file\n", stderr);
                    }
                }
                if (success && !rv_output_publish(&output)) {
                    fputs("ruvac: error: cannot publish output file\n", stderr);
                    success = false;
                }
                rv_output_discard(&output);
            }
        }
    }
    rv_checked_program_destroy(&program);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

#ifdef _WIN32
int wmain(int argc, wchar_t **wide_arguments) {
    char **arguments = calloc((size_t)argc + 1, sizeof(*arguments));
    if (arguments == NULL) {
        return EXIT_FAILURE;
    }
    int result = EXIT_FAILURE;
    for (int index = 0; index < argc; ++index) {
        arguments[index] = rv_windows_utf8(wide_arguments[index]);
        if (arguments[index] == NULL) {
            fputs("ruvac: error: cannot decode command-line arguments\n", stderr);
            goto cleanup;
        }
    }
    result = run(argc, arguments);

cleanup:
    for (int index = 0; index < argc; ++index) {
        free(arguments[index]);
    }
    free(arguments);
    return result;
}
#else
int main(int argc, char **argv) {
    return run(argc, argv);
}
#endif
