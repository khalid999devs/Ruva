#ifndef RUVA_PARSER_TEST_H
#define RUVA_PARSER_TEST_H

#include "tests/foundation/test.h"
#include "compiler/frontend/parser/parser.h"

typedef struct {
    RvSource *source;
    RvAstModule module;
} TestProgram;

bool test_parse_program(const char *input, bool entry, TestProgram *program);
void test_program_destroy(TestProgram *program);
bool test_parse_error(const char *input, bool entry, RvParseErrorKind kind, const char *message);
RvAstExpr *test_initializer(const TestProgram *program, size_t index);
bool test_parser_expressions(void);
bool test_parser_declarations(void);
bool test_parser_statements(void);
bool test_parser_ambiguity(void);
bool test_parser_failures(void);

#endif
