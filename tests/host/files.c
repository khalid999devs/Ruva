#include "test.h"
#include "compiler/common/host/files.h"

#include <stdlib.h>
#include <string.h>

bool test_host_files(void) {
    const char *source = "compiler/common/host/files.c";
    char *canonical = rv_host_canonical_path(source);
    CHECK(canonical != NULL);
    CHECK(strstr(canonical, "/compiler/common/host/files.c") != NULL);
    CHECK(rv_host_regular_file(source) && rv_host_regular_file(canonical));
    CHECK(rv_host_same_file(source, canonical));
    CHECK(!rv_host_same_file(source, "compiler/common/host/numeric.c"));
    CHECK(!rv_host_same_file(source, "build/no-such-host-input"));
    CHECK(!rv_host_regular_file("compiler/common/host"));
    CHECK(!rv_host_regular_file("build/no-such-host-input"));
    FILE *stream = rv_host_fopen(canonical, "rb");
    CHECK(stream != NULL && fgetc(stream) == '#');
    CHECK(fclose(stream) == 0);
    free(canonical);
    CHECK(rv_host_canonical_path("build/no-such-host-input") == NULL);
    return true;
}
