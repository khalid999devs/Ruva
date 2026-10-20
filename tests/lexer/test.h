#ifndef RUVA_LEXER_TEST_H
#define RUVA_LEXER_TEST_H

#include "tests/foundation/test.h"
#include "compiler/frontend/lexer/lexer.h"

bool test_tokens(void);
bool test_text(void);
bool test_lexer_failure(void);
bool test_lex_error(const void *bytes, size_t length, RvLexErrorKind kind,
                    size_t start, size_t end);

#endif
