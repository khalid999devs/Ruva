#include "internal.h"
#include "../../compiler/common/host/files.h"

static bool write_file(const RvText *path, const RvText *text, const char *mode) {
    FILE *stream = rv_host_fopen(rv_text_bytes(path), mode);
    if (stream == NULL) {
        return false;
    }
    size_t length = rv_text_length(text);
    bool success = fwrite(rv_text_bytes(text), 1, length, stream) == length;
    if (fflush(stream) != 0 || ferror(stream)) {
        success = false;
    }
    if (fclose(stream) != 0) {
        success = false;
    }
    return success;
}

bool rv_file_write(RvRuntime *runtime, const RvText *path, const RvText *text, RvLocation where) {
    (void)runtime;
    (void)where;
    return write_file(path, text, "wb");
}

bool rv_file_append(RvRuntime *runtime, const RvText *path, const RvText *text, RvLocation where) {
    (void)runtime;
    (void)where;
    return write_file(path, text, "ab");
}

RvText *rv_file_read(RvRuntime *runtime, const RvText *path, RvLocation where) {
    RvFile *file = rv_file_create(runtime, path, RV_FILE_READ, where);
    RvText *text = rv_file_read_from(runtime, file, where);
    bool success = rv_file_close(runtime, file, where);
    /* The temporary reader stays at the head until this call finishes. */
    runtime->files = file->next;
    rv_release(runtime, file);
    if (!success) {
        rv_panic(runtime, "file close failed", where);
    }
    return text;
}

bool rv_file_exists(RvRuntime *runtime, const RvText *path, RvLocation where) {
    (void)runtime;
    (void)where;
    return rv_host_regular_file(rv_text_bytes(path));
}
