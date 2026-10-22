#ifndef RUVA_SEMANTIC_TEST_H
#define RUVA_SEMANTIC_TEST_H

#include "tests/foundation/test.h"
#include "compiler/analysis/semantics/check.h"

typedef struct {
    char directory[128];
    char entry[256];
} TestProject;

typedef struct {
    const char *source;
    RvCheckErrorKind kind;
    const char *message;
} SemanticCase;

bool test_project_init(TestProject *project);
bool test_project_write(const TestProject *project, const char *name, const char *source);
bool test_project_remove(TestProject *project);
bool test_accept(const char *source, RvCheckedProgram *program);
bool test_semantic_cases(const SemanticCase *cases, size_t count);
RvAstStmt *test_statement(RvCheckedProgram *program, size_t index);
RvSymbol *test_symbol(RvCheckedProgram *program, const char *name, RvSymbolKind kind);
bool test_semantic_expressions(void);
bool test_semantic_records(void);
bool test_semantic_ownership(void);
bool test_semantic_names(void);
bool test_semantic_modules(void);
bool test_semantic_failures(void);

#endif
