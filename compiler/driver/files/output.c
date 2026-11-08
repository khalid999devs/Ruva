#define _XOPEN_SOURCE 700

#include "output.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

bool rv_output_open(RvOutput *output, const char *destination) {
    *output = (RvOutput){.destination = destination};
    struct stat info;
    if (lstat(destination, &info) == 0 && !S_ISREG(info.st_mode)) {
        errno = EINVAL;
        return false;
    }
    const char *slash = strrchr(destination, '/');
    size_t directory = slash == NULL ? 0 : (size_t)(slash - destination) + 1;
    const char suffix[] = ".ruva-output-XXXXXX";
    if (directory > SIZE_MAX - sizeof(suffix)) {
        errno = ENOMEM;
        return false;
    }
    output->temporary = malloc(directory + sizeof(suffix));
    if (output->temporary == NULL) {
        return false;
    }
    memcpy(output->temporary, destination, directory);
    memcpy(output->temporary + directory, suffix, sizeof(suffix));
    int descriptor = mkstemp(output->temporary);
    if (descriptor < 0) {
        rv_output_discard(output);
        return false;
    }
    output->created = true;
    output->stream = fdopen(descriptor, "wb");
    if (output->stream == NULL) {
        close(descriptor);
        rv_output_discard(output);
        return false;
    }
    return true;
}

bool rv_output_close(RvOutput *output) {
    if (output->stream == NULL) {
        return true;
    }
    FILE *stream = output->stream;
    output->stream = NULL;
    return fclose(stream) == 0;
}

bool rv_output_publish(RvOutput *output) {
    if (!rv_output_close(output)) {
        return false;
    }
    if (rename(output->temporary, output->destination) != 0) {
        return false;
    }
    output->created = false;
    return true;
}

void rv_output_discard(RvOutput *output) {
    rv_output_close(output);
    if (output->temporary != NULL) {
        if (output->created) {
            unlink(output->temporary);
        }
        free(output->temporary);
        output->temporary = NULL;
        output->created = false;
    }
}
