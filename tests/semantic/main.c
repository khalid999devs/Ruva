#include "test.h"

#include <stdlib.h>

int main(void) {
    if (!test_semantic_expressions() || !test_semantic_records()
        || !test_semantic_ownership() || !test_semantic_names()
        || !test_semantic_modules() || !test_semantic_failures()) {
        return EXIT_FAILURE;
    }
    puts("semantic checks passed");
    return EXIT_SUCCESS;
}
