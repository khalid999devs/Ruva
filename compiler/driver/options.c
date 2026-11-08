#include "options.h"

#include <string.h>

void rv_options_usage(FILE *output) {
    fputs("usage: ruvac [--check | --emit-c] [--sanitize] source [-o output]\n", output);
}

bool rv_options_parse(int argc, char **argv, RvOptions *options, const char **error) {
    *options = (RvOptions){0};
    *error = NULL;
    bool positional = false;
    for (int index = 1; index < argc; ++index) {
        const char *argument = argv[index];
        if (!positional && strcmp(argument, "--") == 0) {
            positional = true;
        } else if (!positional && strcmp(argument, "--help") == 0 && argc == 2) {
            options->help = true;
            return true;
        } else if (!positional && strcmp(argument, "--check") == 0 && !options->check) {
            options->check = true;
        } else if (!positional && strcmp(argument, "--emit-c") == 0 && !options->emit_c) {
            options->emit_c = true;
        } else if (!positional && strcmp(argument, "--sanitize") == 0 && !options->sanitize) {
            options->sanitize = true;
        } else if (!positional && strcmp(argument, "-o") == 0 && options->output == NULL
                   && index + 1 < argc) {
            options->output = argv[++index];
        } else if (!positional && argument[0] == '-') {
            *error = "unknown, repeated or incomplete option";
            return false;
        } else if (options->source == NULL && argument[0] != '\0') {
            options->source = argument;
        } else {
            *error = "exactly one source path is required";
            return false;
        }
    }
    if (options->source == NULL) {
        *error = "a source path is required";
    } else if (options->check && (options->emit_c || options->output != NULL || options->sanitize)) {
        *error = "--check cannot be combined with output or native options";
    } else if (options->emit_c && options->sanitize) {
        *error = "--sanitize requires native compilation";
    } else if (options->output != NULL && options->output[0] == '\0') {
        *error = "output path must not be empty";
    }
    if (options->output == NULL) {
#ifdef _WIN32
        options->output = options->emit_c ? "out.c" : "a.exe";
#else
        options->output = options->emit_c ? "out.c" : "a.out";
#endif
    }
    return *error == NULL;
}
