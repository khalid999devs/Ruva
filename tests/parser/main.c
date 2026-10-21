#include "test.h"

#include <stdlib.h>

int main(void) {
    if (!test_parser_expressions()
        || !test_parser_declarations()
        || !test_parser_statements()
        || !test_parser_ambiguity()
        || !test_parser_failures()) {
        return EXIT_FAILURE;
    }

    puts("Parser tests passed.");
    return EXIT_SUCCESS;
}
