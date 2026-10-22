#include "source.h"
#include "span.h"
#include "compiler/common/memory/vector.h"
#include "compiler/common/host/files.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

struct RvSource {
    RvAllocator allocator;
    char *path;
    unsigned char *bytes;
    size_t length;
    RvVector lines;
};

static void set_error(RvSourceError *error, RvSourceErrorKind kind, int system_error) {
    if (error != NULL) {
        *error = (RvSourceError){kind, system_error};
    }
}

static bool index_lines(RvSource *source) {
    size_t *start = rv_vector_append(&source->lines);
    if (start == NULL) {
        return false;
    }
    *start = 0;
    for (size_t index = 0; index < source->length; ++index) {
        unsigned char byte = source->bytes[index];
        if (byte != '\n' && byte != '\r') {
            continue;
        }
        if (byte == '\r' && index + 1 < source->length && source->bytes[index + 1] == '\n') {
            ++index;
        }
        start = rv_vector_append(&source->lines);
        if (start == NULL) {
            return false;
        }
        *start = index + 1;
    }
    return true;
}

RvSource *rv_source_from_bytes(RvAllocator allocator, const char *path,
                               const void *bytes, size_t length, RvSourceError *error) {
    set_error(error, RV_SOURCE_OK, 0);
    if (path == NULL || path[0] == '\0' || (bytes == NULL && length != 0)) {
        set_error(error, RV_SOURCE_INVALID, 0);
        return NULL;
    }
    size_t byte_size;
    size_t path_size;
    if (!rv_size_add(length, 1, &byte_size)
        || !rv_size_add(strlen(path), 1, &path_size)) {
        set_error(error, RV_SOURCE_TOO_LARGE, 0);
        return NULL;
    }
    RvSource *source = allocator.allocate(allocator.context, sizeof(*source));
    if (source == NULL) {
        set_error(error, RV_SOURCE_ALLOCATION, 0);
        return NULL;
    }
    *source = (RvSource){.allocator = allocator, .length = length};
    rv_vector_init(&source->lines, allocator, sizeof(size_t));
    source->path = allocator.allocate(allocator.context, path_size);
    if (source->path == NULL) {
        goto allocation_failure;
    }
    source->bytes = allocator.allocate(allocator.context, byte_size);
    if (source->bytes == NULL) {
        goto allocation_failure;
    }
    memcpy(source->path, path, path_size);
    if (length != 0) {
        memcpy(source->bytes, bytes, length);
    }
    source->bytes[length] = '\0';
    if (!index_lines(source)) {
        goto allocation_failure;
    }
    return source;

allocation_failure:
    rv_source_destroy(source);
    set_error(error, RV_SOURCE_ALLOCATION, 0);
    return NULL;
}

RvSource *rv_source_load(RvAllocator allocator, const char *path, RvSourceError *error) {
    set_error(error, RV_SOURCE_OK, 0);
    if (path == NULL || path[0] == '\0') {
        set_error(error, RV_SOURCE_INVALID, 0);
        return NULL;
    }
    errno = 0;
    FILE *stream = rv_host_fopen(path, "rb");
    if (stream == NULL) {
        set_error(error, RV_SOURCE_IO, errno);
        return NULL;
    }
    RvVector bytes;
    rv_vector_init(&bytes, allocator, 1);
    RvSourceError failure = {RV_SOURCE_OK, 0};
    for (;;) {
        size_t required;
        if (!rv_size_add(bytes.length, 4096, &required)) {
            failure.kind = RV_SOURCE_TOO_LARGE;
            break;
        }
        if (!rv_vector_reserve(&bytes, required)) {
            failure.kind = RV_SOURCE_ALLOCATION;
            break;
        }
        errno = 0;
        size_t count = fread(bytes.data + bytes.length, 1, 4096, stream);
        bytes.length += count;
        if (count < 4096) {
            if (ferror(stream)) {
                failure = (RvSourceError){RV_SOURCE_IO, errno};
            }
            break;
        }
    }
    errno = 0;
    if (fclose(stream) != 0 && failure.kind == RV_SOURCE_OK) {
        failure = (RvSourceError){RV_SOURCE_IO, errno};
    }
    RvSource *source = NULL;
    if (failure.kind == RV_SOURCE_OK) {
        source = rv_source_from_bytes(allocator, path, bytes.data, bytes.length, error);
    } else {
        set_error(error, failure.kind, failure.system_error);
    }
    rv_vector_destroy(&bytes);
    return source;
}

void rv_source_destroy(RvSource *source) {
    if (source == NULL) {
        return;
    }
    RvAllocator allocator = source->allocator;
    rv_vector_destroy(&source->lines);
    if (source->path != NULL) {
        allocator.release(allocator.context, source->path);
    }
    if (source->bytes != NULL) {
        allocator.release(allocator.context, source->bytes);
    }
    allocator.release(allocator.context, source);
}

const char *rv_source_path(const RvSource *source) {
    return source->path;
}

const unsigned char *rv_source_bytes(const RvSource *source) {
    return source->bytes;
}

size_t rv_source_length(const RvSource *source) {
    return source->length;
}

size_t rv_source_line_count(const RvSource *source) {
    return source->lines.length;
}

bool rv_source_position(const RvSource *source, size_t offset, RvSourcePosition *position) {
    if (source == NULL || position == NULL || offset > source->length) {
        return false;
    }
    const size_t *lines = (const size_t *)source->lines.data;
    size_t low = 0;
    size_t high = source->lines.length;
    while (low + 1 < high) {
        size_t middle = low + (high - low) / 2;
        if (lines[middle] <= offset) {
            low = middle;
        } else {
            high = middle;
        }
    }
    *position = (RvSourcePosition){low + 1, offset - lines[low] + 1};
    return true;
}

bool rv_source_line(const RvSource *source, size_t line, RvSourceLine *result) {
    if (source == NULL || result == NULL || line == 0 || line > source->lines.length) {
        return false;
    }
    const size_t *lines = (const size_t *)source->lines.data;
    size_t start = lines[line - 1];
    size_t end = line < source->lines.length ? lines[line] : source->length;
    if (end > start && source->bytes[end - 1] == '\n') {
        --end;
    }
    if (end > start && source->bytes[end - 1] == '\r') {
        --end;
    }
    *result = (RvSourceLine){source->bytes + start, end - start};
    return true;
}

bool rv_span_create(RvSpan *span, const RvSource *source, size_t start, size_t end) {
    if (span == NULL || source == NULL || start > end || end > source->length) {
        return false;
    }
    *span = (RvSpan){source, start, end};
    return true;
}
