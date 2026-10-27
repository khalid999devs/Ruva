#include "internal.h"
#include "../io/read.h"
#include "../../compiler/common/host/files.h"

#include <string.h>

RvFile *rv_file_create(RvRuntime *runtime, const RvText *path,
                       RvFileMode mode, RvLocation where) {
    RvFile *file = rv_allocate(runtime, sizeof(*file), where);
    *file = (RvFile){.mode = mode, .location = where, .next = runtime->files};
    runtime->files = file;
    const char *spelling = "rb";
    if (mode == RV_FILE_WRITE) {
        spelling = "wb";
    } else if (mode == RV_FILE_APPEND) {
        spelling = "ab";
    }
    file->stream = rv_host_fopen(rv_text_bytes(path), spelling);
    if (file->stream == NULL) {
        rv_panic(runtime, "cannot open file", where);
    }
    return file;
}

RvFile *rv_file_open(RvRuntime *runtime, const RvText *path, const RvText *mode, RvLocation where) {
    RvFileMode access;
    const char *bytes = rv_text_bytes(mode);
    if (strcmp(bytes, "read") == 0) {
        access = RV_FILE_READ;
    } else if (strcmp(bytes, "write") == 0) {
        access = RV_FILE_WRITE;
    } else if (strcmp(bytes, "append") == 0) {
        access = RV_FILE_APPEND;
    } else {
        rv_panic(runtime, "invalid file mode", where);
    }
    return rv_file_create(runtime, path, access, where);
}

static void require_open(RvRuntime *runtime, const RvFile *file, RvLocation where) {
    if (file->stream == NULL) {
        rv_panic(runtime, "file handle is closed", where);
    }
}

RvText *rv_file_read_from(RvRuntime *runtime, RvFile *file, RvLocation where) {
    require_open(runtime, file, where);
    if (file->mode != RV_FILE_READ) {
        rv_panic(runtime, "file handle is not readable", where);
    }
    return rv_read_text(runtime, file->stream, false, where);
}

bool rv_file_write_to(RvRuntime *runtime, RvFile *file, const RvText *text, RvLocation where) {
    require_open(runtime, file, where);
    if (file->mode == RV_FILE_READ) {
        rv_panic(runtime, "file handle is not writable", where);
    }
    size_t length = rv_text_length(text);
    bool success = fwrite(rv_text_bytes(text), 1, length, file->stream) == length;
    if (fflush(file->stream) != 0 || ferror(file->stream)) {
        success = false;
    }
    return success;
}

bool rv_file_close(RvRuntime *runtime, RvFile *file, RvLocation where) {
    require_open(runtime, file, where);
    FILE *stream = file->stream;
    file->stream = NULL;
    return fclose(stream) == 0;
}

bool rv_files_cleanup(RvRuntime *runtime) {
    bool success = true;
    for (RvFile *file = runtime->files; file != NULL; file = file->next) {
        if (file->stream != NULL) {
            FILE *stream = file->stream;
            file->stream = NULL;
            if (fclose(stream) != 0) {
                rv_report_error(runtime, "file close failed", file->location);
                success = false;
            }
        }
    }
    runtime->files = NULL;
    return success;
}
