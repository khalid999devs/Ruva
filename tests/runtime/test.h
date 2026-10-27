#ifndef RUVA_RUNTIME_TEST_H
#define RUVA_RUNTIME_TEST_H

#include "tests/foundation/test.h"
#include "runtime/ruva_runtime.h"

bool test_runtime_memory(void);
bool test_runtime_text(void);
bool test_runtime_numbers(void);
bool test_runtime_arrays(void);
bool test_runtime_conversions(void);
bool test_runtime_input(void);
bool test_runtime_files(void);
bool test_runtime_write_failures(RvRuntime *runtime, const char *directory, RvLocation where);
bool test_runtime_text_failure(RvRuntime *runtime, const char *mode, RvLocation where);
bool test_runtime_io_failure(RvRuntime *runtime, const char *mode, RvLocation where);
bool test_runtime_array_failure(RvRuntime *runtime, TestAllocator *allocations,
                                 const char *mode, RvLocation where);

#endif
