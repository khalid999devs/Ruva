#include "test.h"

#include <stdlib.h>

int main(void) {
    if (!test_memory() || !test_source()) {
        return EXIT_FAILURE;
    }
    puts("Foundation tests passed.");
    return EXIT_SUCCESS;
}
