#include "test.h"
#include "compiler/common/host/streams.h"

#include <stdlib.h>

int main(void) {
    if (!rv_host_initialize_streams() || !test_host_numbers()
        || !test_host_arguments() || !test_host_files()) {
        return EXIT_FAILURE;
    }
    puts("Host tests passed.");
    return EXIT_SUCCESS;
}
