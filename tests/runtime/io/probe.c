#define _XOPEN_SOURCE 700

#include "../test.h"
#include "runtime/files/internal.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool test_runtime_io_failure(RvRuntime *runtime, const char *mode, RvLocation where) {
    bool input = strcmp(mode, "input-read-error") == 0;
    bool read = strcmp(mode, "file-read-error") == 0;
    bool finish = strcmp(mode, "final-close-error") == 0;
    if (!input && !read && !finish) {
        return false;
    }
    RvFile *file = rv_allocate(runtime, sizeof(*file), where);
    *file = (RvFile){.mode = RV_FILE_READ, .location = where, .next = runtime->files};
    runtime->files = file;
    if (input) {
        int descriptors[2];
        if (pipe(descriptors) != 0) {
            rv_panic(runtime, "cannot prepare read failure", where);
        }
        close(descriptors[0]);
        file->stream = fdopen(descriptors[1], "wb");
        if (file->stream == NULL) {
            close(descriptors[1]);
            rv_panic(runtime, "cannot prepare read failure", where);
        }
        runtime->input = file->stream;
        rv_ask_text(runtime, where);
    } else {
        file->stream = tmpfile();
        if (file->stream == NULL || close(fileno(file->stream)) != 0) {
            rv_panic(runtime, "cannot prepare close failure", where);
        }
        if (read) {
            rv_file_read_from(runtime, file, where);
        } else {
            exit(rv_runtime_finish(runtime) ? EXIT_SUCCESS : EXIT_FAILURE);
        }
    }
    return true;
}
