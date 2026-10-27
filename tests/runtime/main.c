#include "test.h"

#include <stdlib.h>

int main(void) {
    if (!test_runtime_memory() || !test_runtime_text() || !test_runtime_numbers()
        || !test_runtime_arrays() || !test_runtime_conversions()
        || !test_runtime_input() || !test_runtime_files()) {
        return EXIT_FAILURE;
    }
    puts("runtime checks passed");
    return EXIT_SUCCESS;
}
