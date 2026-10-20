#include "test.h"

#include <stdlib.h>

int main(void) {
    if (!test_tokens() || !test_text() || !test_lexer_failure()) {
        return EXIT_FAILURE;
    }

    puts("Lexer tests passed.");
    return EXIT_SUCCESS;
}
