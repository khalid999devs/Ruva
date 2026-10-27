#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE

#include "../test.h"
#include "runtime/files/internal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool test_runtime_files(void) {
    char directory[] = "/tmp/ruva-runtime-files-XXXXXX";
    CHECK(mkdtemp(directory) != NULL);
    char path[256];
    int length = snprintf(path, sizeof(path), "%s/data.txt", directory);
    CHECK(length > 0 && (size_t)length < sizeof(path));
    RvRuntime runtime;
    FILE *errors = tmpfile();
    CHECK(errors != NULL);
    rv_runtime_init(&runtime, stdout, errors);
    RvLocation where = {"files.ruv", 2, 3};
    RvText *name = rv_text_create(&runtime, path, (size_t)length, where);
    RvText *write = rv_text_create(&runtime, "write", 5, where);
    RvText *read = rv_text_create(&runtime, "read", 4, where);
    RvText *chunk = rv_text_create(&runtime, "a\r\n", 3, where);
    CHECK(!rv_file_exists(&runtime, name, where));
    RvFile *writer = rv_file_open(&runtime, name, write, where);
    CHECK(rv_file_write_to(&runtime, writer, chunk, where));
    CHECK(rv_file_write_to(&runtime, writer, chunk, where));
    CHECK(rv_file_close(&runtime, writer, where));
    CHECK(writer->stream == NULL);
    RvText *contents = rv_file_read(&runtime, name, where);
    CHECK(strcmp(rv_text_bytes(contents), "a\r\na\r\n") == 0);
    rv_text_release(&runtime, contents);
    RvFile *reader = rv_file_open(&runtime, name, read, where);
    RvText *remaining = rv_file_read_from(&runtime, reader, where);
    CHECK(rv_text_length(remaining) == 6);
    rv_text_release(&runtime, remaining);
    remaining = rv_file_read_from(&runtime, reader, where);
    CHECK(rv_text_length(remaining) == 0);
    rv_text_release(&runtime, remaining);
    int read_descriptor = fileno(reader->stream);

    RvFile *bad_close = rv_file_open(&runtime, name, read, where);
    CHECK(close(fileno(bad_close->stream)) == 0);
    CHECK(!rv_file_close(&runtime, bad_close, where));
    CHECK(bad_close->stream == NULL);

    int descriptors[2];
    CHECK(pipe(descriptors) == 0);
    CHECK(close(descriptors[0]) == 0);
    RvFile *broken = rv_allocate(&runtime, sizeof(*broken), where);
    *broken = (RvFile){.stream = fdopen(descriptors[1], "wb"), .mode = RV_FILE_WRITE,
                      .location = where, .next = runtime.files};
    runtime.files = broken;
    CHECK(broken->stream != NULL);
    CHECK(!rv_file_write_to(&runtime, broken, chunk, where));
    (void)rv_file_close(&runtime, broken, where);
    CHECK(broken->stream == NULL);

    CHECK(test_runtime_write_failures(&runtime, directory, where));

    RvFile *final_close = rv_file_open(&runtime, name, read, where);
    CHECK(close(fileno(final_close->stream)) == 0);
    CHECK(!rv_runtime_finish(&runtime));
    CHECK(runtime.files == NULL && runtime.live_allocations == 0);
    errno = 0;
    CHECK(fcntl(read_descriptor, F_GETFD) == -1 && errno == EBADF);
    rewind(errors);
    char message[128] = {0};
    CHECK(fread(message, 1, sizeof(message) - 1, errors) > 0);
    CHECK(strcmp(message, "files.ruv:2:3: error: file close failed\n") == 0);
    CHECK(rv_runtime_cleanup(&runtime));
    CHECK(fclose(errors) == 0);
    CHECK(unlink(path) == 0 && rmdir(directory) == 0);
    return true;
}
