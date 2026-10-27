#define _XOPEN_SOURCE 700

#include "../test.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static size_t descriptor_count(void) {
    size_t count = 0;
    for (int descriptor = 0; descriptor < 256; ++descriptor) {
        if (fcntl(descriptor, F_GETFD) != -1) {
            ++count;
        }
    }
    return count;
}

bool test_runtime_write_failures(RvRuntime *runtime, const char *directory, RvLocation where) {
    char path[256];
    int length = snprintf(path, sizeof(path), "%s/pipe", directory);
    CHECK(length > 0 && (size_t)length < sizeof(path));
    CHECK(mkfifo(path, 0600) == 0);
    RvText *name = rv_text_create(runtime, path, (size_t)length, where);
    size_t size = 4 * 1024 * 1024;
    char *bytes = rv_allocate(runtime, size, where);
    memset(bytes, 'x', size);
    RvText *text = rv_text_create(runtime, bytes, size, where);
    rv_release(runtime, bytes);
    size_t descriptors = descriptor_count();

    for (size_t index = 0; index < 2; ++index) {
        pid_t child = fork();
        CHECK(child >= 0);
        if (child == 0) {
            int input = open(path, O_RDONLY);
            if (input < 0) {
                _Exit(2);
            }
            unsigned char byte = 0;
            ssize_t received = read(input, &byte, 1);
            close(input);
            if (received != 1 || byte != 'x') {
                _Exit(3);
            }
            _Exit(0);
        }
        bool success;
        if (index == 0) {
            success = rv_file_write(runtime, name, text, where);
        } else {
            success = rv_file_append(runtime, name, text, where);
        }
        int status;
        pid_t waited;
        do {
            waited = waitpid(child, &status, 0);
        } while (waited < 0 && errno == EINTR);
        CHECK(waited == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
        CHECK(!success && descriptor_count() == descriptors);
    }

    rv_text_release(runtime, name);
    rv_text_release(runtime, text);
    CHECK(unlink(path) == 0);
    return true;
}
