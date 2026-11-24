CC ?= cc
AR ?= ar
PYTHON ?= python3
FLEX ?= flex
BUILD_DIR ?= build/debug
SANITIZE_BUILD_DIR ?= build/sanitize
CPPFLAGS ?=
CFLAGS ?= -O0 -g
LDFLAGS ?=
LDLIBS ?=
WARNINGS = -std=c11 -Wall -Wextra -Wpedantic -Werror
TARGET_TRIPLE := $(shell $(CC) -dumpmachine)
WINDOWS_TARGET := $(findstring mingw,$(TARGET_TRIPLE))
HOST_SOURCES = compiler/common/host/files.c compiler/common/host/numeric.c \
    compiler/common/host/streams.c
TOOLCHAIN_SOURCES = compiler/driver/toolchain/arguments.c
ifneq ($(WINDOWS_TARGET),)
EXE_SUFFIX = .exe
HOST_SOURCES += compiler/common/host/win32/utf8.c
TOOLCHAIN_SOURCES += compiler/driver/toolchain/windows.c \
    compiler/driver/files/windows.c compiler/driver/files/temporary.c
DRIVER_LDFLAGS = -municode
DRIVER_LIBS = -lbcrypt
else
TOOLCHAIN_SOURCES += compiler/driver/toolchain/posix.c compiler/driver/files/output.c
endif

FOUNDATION_SOURCES = compiler/common/memory/allocator.c \
    compiler/common/memory/arena.c compiler/common/memory/vector.c \
    compiler/common/source/source.c compiler/common/diagnostics/diagnostic.c $(HOST_SOURCES)
TEST_SOURCES = tests/foundation/main.c tests/foundation/support.c \
    tests/foundation/memory.c tests/foundation/source.c
PROBE_SOURCES = tests/foundation/probe.c tests/foundation/support.c
FOUNDATION_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(FOUNDATION_SOURCES))
TEST_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SOURCES))
PROBE_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(PROBE_SOURCES))
LIBRARY = $(BUILD_DIR)/lib/libruva_foundation.a
FOUNDATION_TEST = $(BUILD_DIR)/tests/foundation/foundation-tests$(EXE_SUFFIX)
FIXTURE_PROBE = $(BUILD_DIR)/tests/foundation/fixture-probe$(EXE_SUFFIX)
LEXER_SOURCES = compiler/frontend/lexer/token.c compiler/frontend/lexer/lexer.c
LEXER_TEST_SOURCES = tests/lexer/main.c tests/lexer/tokens.c \
    tests/lexer/text.c tests/lexer/failure.c tests/foundation/support.c
LEXER_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(LEXER_SOURCES)) \
    $(BUILD_DIR)/compiler/frontend/lexer/scanner.o
LEXER_TEST_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(LEXER_TEST_SOURCES))
LEXER_PROBE_OBJECT = $(BUILD_DIR)/tests/lexer/probe.o
LEXER_LIBRARY = $(BUILD_DIR)/lib/libruva_lexer.a
LEXER_TEST = $(BUILD_DIR)/tests/lexer/lexer-tests$(EXE_SUFFIX)
LEXER_PROBE = $(BUILD_DIR)/tests/lexer/lexer-probe$(EXE_SUFFIX)
SCANNER_SOURCE = $(BUILD_DIR)/compiler/frontend/lexer/scanner.c
PARSER_SOURCES = compiler/frontend/ast/ast.c compiler/frontend/ast/dump.c \
    compiler/frontend/parser/parser.c compiler/frontend/parser/expression.c \
    compiler/frontend/parser/statement.c compiler/frontend/parser/declaration.c
PARSER_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(PARSER_SOURCES))
PARSER_LIBRARY = $(BUILD_DIR)/lib/libruva_parser.a
PARSER_TEST_SOURCES = tests/parser/main.c tests/parser/expressions.c \
    tests/parser/declarations.c tests/parser/statements.c tests/parser/ambiguity.c \
    tests/parser/support/helpers.c tests/parser/support/failures.c tests/foundation/support.c
PARSER_TEST_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(PARSER_TEST_SOURCES))
PARSER_TEST = $(BUILD_DIR)/tests/parser/parser-tests$(EXE_SUFFIX)
PARSER_PROBE_OBJECT = $(BUILD_DIR)/tests/parser/tools/probe.o
PARSER_PROBE = $(BUILD_DIR)/tests/parser/parser-probe$(EXE_SUFFIX)
ANALYSIS_SOURCES = compiler/analysis/types/type.c compiler/analysis/diagnostics/error.c \
    compiler/analysis/modules/path.c compiler/analysis/modules/loader.c \
    compiler/analysis/semantics/check.c compiler/analysis/semantics/scope.c \
    compiler/analysis/semantics/declarations.c compiler/analysis/semantics/statement.c \
    compiler/analysis/semantics/expressions/expression.c \
    compiler/analysis/semantics/expressions/call.c \
    compiler/analysis/semantics/expressions/record.c \
    compiler/analysis/semantics/expressions/literal.c
ANALYSIS_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(ANALYSIS_SOURCES))
ANALYSIS_LIBRARY = $(BUILD_DIR)/lib/libruva_analysis.a
SEMANTIC_TEST_SOURCES = tests/semantic/main.c tests/semantic/expressions.c \
    tests/semantic/records.c tests/semantic/ownership.c tests/semantic/names.c \
    tests/semantic/support/helpers.c tests/semantic/modules/modules.c \
    tests/semantic/failure/failure.c tests/foundation/support.c
SEMANTIC_TEST_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SEMANTIC_TEST_SOURCES))
SEMANTIC_TEST = $(BUILD_DIR)/tests/semantic/semantic-tests
SEMANTIC_PROBE_OBJECT = $(BUILD_DIR)/tests/semantic/tools/probe.o
SEMANTIC_PROBE = $(BUILD_DIR)/tests/semantic/semantic-probe$(EXE_SUFFIX)

RUNTIME_SOURCES = runtime/core/runtime.c runtime/core/memory.c runtime/core/numeric.c \
    runtime/text/text.c runtime/text/format.c runtime/text/output.c \
    runtime/text/snapshot.c runtime/collections/array.c \
    runtime/conversion/parse.c runtime/conversion/numeric.c \
    runtime/io/read.c runtime/io/input.c runtime/files/handle.c runtime/files/path.c $(HOST_SOURCES)
RUNTIME_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(RUNTIME_SOURCES))
RUNTIME_LIBRARY = $(BUILD_DIR)/lib/libruva_runtime.a
BACKEND_SOURCES = compiler/backend/c/emit.c compiler/backend/c/literal.c \
    compiler/backend/c/expression.c compiler/backend/c/validation.c \
    compiler/backend/c/statements/block.c compiler/backend/c/statements/control.c \
    compiler/backend/c/functions/declaration.c compiler/backend/c/functions/call.c \
    compiler/backend/c/functions/builtin.c \
    compiler/backend/c/collections/ownership.c compiler/backend/c/collections/record.c \
    compiler/backend/c/collections/array.c compiler/backend/c/collections/assignment.c \
    compiler/backend/c/collections/walk.c
BACKEND_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(BACKEND_SOURCES))
BACKEND_LIBRARY = $(BUILD_DIR)/lib/libruva_backend.a
DRIVER_SOURCES = compiler/driver/main.c compiler/driver/options.c \
    compiler/driver/toolchain/toolchain.c $(TOOLCHAIN_SOURCES)
DRIVER_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(DRIVER_SOURCES))
COMPILER = $(BUILD_DIR)/bin/ruvac$(EXE_SUFFIX)
HOST_TEST_SOURCES = tests/host/main.c tests/host/numeric.c tests/host/arguments.c \
    tests/host/files.c tests/foundation/support.c compiler/driver/toolchain/arguments.c
HOST_TEST_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(HOST_TEST_SOURCES))
HOST_TEST = $(BUILD_DIR)/tests/host/host-tests$(EXE_SUFFIX)

RUNTIME_TEST_SOURCES = tests/runtime/main.c tests/runtime/memory.c \
    tests/runtime/text.c tests/runtime/numeric.c tests/runtime/collections/array.c \
    tests/runtime/io/conversion.c tests/runtime/io/input.c tests/runtime/io/files.c \
    tests/runtime/io/write.c \
    tests/foundation/support.c
RUNTIME_TEST_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(RUNTIME_TEST_SOURCES))
RUNTIME_TEST = $(BUILD_DIR)/tests/runtime/runtime-tests
RUNTIME_PROBE_OBJECT = $(BUILD_DIR)/tests/runtime/probe.o \
    $(BUILD_DIR)/tests/runtime/collections/probe.o $(BUILD_DIR)/tests/runtime/io/probe.o \
    $(BUILD_DIR)/tests/runtime/failure/text.o
RUNTIME_PROBE = $(BUILD_DIR)/tests/runtime/runtime-probe
BACKEND_TEST_SOURCES = tests/backend/main.c tests/semantic/support/helpers.c tests/foundation/support.c
BACKEND_TEST_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(BACKEND_TEST_SOURCES))
BACKEND_TEST = $(BUILD_DIR)/tests/backend/backend-tests
NATIVE_SANITIZE ?= 0

.PHONY: all test test-foundation test-fixtures test-lexer test-lexer-fixtures \
    test-parser test-parser-fixtures test-semantic test-semantic-fixtures \
    test-runtime test-runtime-fixtures test-backend test-native test-integration \
    test-python check-repository sanitize clean compiler windows-build windows-test test-host test-programs

ifneq ($(WINDOWS_TARGET),)
all: windows-build
else
all: $(LIBRARY) $(FOUNDATION_TEST) $(FIXTURE_PROBE) $(LEXER_LIBRARY) \
    $(LEXER_TEST) $(LEXER_PROBE) $(PARSER_LIBRARY) $(PARSER_TEST) $(PARSER_PROBE) \
    $(ANALYSIS_LIBRARY) $(SEMANTIC_TEST) $(SEMANTIC_PROBE) \
    $(RUNTIME_LIBRARY) $(BACKEND_LIBRARY) $(COMPILER) \
    $(RUNTIME_TEST) $(RUNTIME_PROBE) $(BACKEND_TEST) $(HOST_TEST)
endif

compiler: $(COMPILER) $(RUNTIME_LIBRARY)

windows-build: compiler $(FOUNDATION_TEST) $(FIXTURE_PROBE) $(LEXER_TEST) $(LEXER_PROBE) \
    $(PARSER_TEST) $(PARSER_PROBE) $(SEMANTIC_PROBE) $(HOST_TEST)

windows-test: windows-build
	$(PYTHON) scripts/windows/verify.py --build-dir $(BUILD_DIR)

$(LIBRARY) $(LEXER_LIBRARY) $(PARSER_LIBRARY) $(ANALYSIS_LIBRARY) \
    $(RUNTIME_LIBRARY) $(BACKEND_LIBRARY): Makefile

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -I. -MMD -MP -c $< -o $@

$(LIBRARY): $(FOUNDATION_OBJECTS)
	@mkdir -p $(dir $@)
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(FOUNDATION_TEST): $(TEST_OBJECTS) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(FIXTURE_PROBE): $(PROBE_OBJECTS) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(SCANNER_SOURCE): compiler/frontend/lexer/scanner.l
	@mkdir -p $(dir $@)
	$(FLEX) -o $@ $<

# Flex 2.6.4 emits a signed buffer-size comparison.
$(BUILD_DIR)/compiler/frontend/lexer/scanner.o: $(SCANNER_SOURCE)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -Wno-sign-compare -I. -MMD -MP -c $< -o $@

$(LEXER_LIBRARY): $(LEXER_OBJECTS)
	@mkdir -p $(dir $@)
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(PARSER_LIBRARY): $(PARSER_OBJECTS)
	@mkdir -p $(dir $@)
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(ANALYSIS_LIBRARY): $(ANALYSIS_OBJECTS)
	@mkdir -p $(dir $@)
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(LEXER_TEST): $(LEXER_TEST_OBJECTS) $(LEXER_LIBRARY) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(LEXER_PROBE): $(LEXER_PROBE_OBJECT) $(LEXER_LIBRARY) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(PARSER_TEST): $(PARSER_TEST_OBJECTS) $(PARSER_LIBRARY) $(LEXER_LIBRARY) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(PARSER_PROBE): $(PARSER_PROBE_OBJECT) $(PARSER_LIBRARY) $(LEXER_LIBRARY) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(SEMANTIC_PROBE): $(SEMANTIC_PROBE_OBJECT) $(ANALYSIS_LIBRARY) $(PARSER_LIBRARY) $(LEXER_LIBRARY) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(SEMANTIC_TEST): $(SEMANTIC_TEST_OBJECTS) $(ANALYSIS_LIBRARY) $(PARSER_LIBRARY) $(LEXER_LIBRARY) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(RUNTIME_LIBRARY): $(RUNTIME_OBJECTS)
	@mkdir -p $(dir $@)
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(BACKEND_LIBRARY): $(BACKEND_OBJECTS)
	@mkdir -p $(dir $@)
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(COMPILER): $(DRIVER_OBJECTS) $(BACKEND_LIBRARY) $(ANALYSIS_LIBRARY) $(PARSER_LIBRARY) $(LEXER_LIBRARY) $(LIBRARY)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(DRIVER_LDFLAGS) $^ $(LDLIBS) $(DRIVER_LIBS) -o $@

$(HOST_TEST): $(HOST_TEST_OBJECTS) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(RUNTIME_TEST): $(RUNTIME_TEST_OBJECTS) $(RUNTIME_LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(RUNTIME_PROBE): $(RUNTIME_PROBE_OBJECT) $(BUILD_DIR)/tests/foundation/support.o $(RUNTIME_LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BACKEND_TEST): $(BACKEND_TEST_OBJECTS) $(BACKEND_LIBRARY) $(ANALYSIS_LIBRARY) $(PARSER_LIBRARY) $(LEXER_LIBRARY) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

ifneq ($(WINDOWS_TARGET),)
test test-programs: windows-test
else
test: check-repository test-programs

test-programs: test-foundation test-fixtures test-lexer test-lexer-fixtures \
    test-parser test-parser-fixtures test-semantic test-semantic-fixtures \
    test-runtime test-runtime-fixtures test-backend test-native test-integration test-python test-host
endif

test-host: $(HOST_TEST)
	$(HOST_TEST)

check-repository:
	$(PYTHON) scripts/repository/check-layout.py
	$(PYTHON) scripts/repository/check-docs.py

test-foundation: $(FOUNDATION_TEST)
	$(FOUNDATION_TEST)

test-fixtures: $(FIXTURE_PROBE)
	$(PYTHON) scripts/testing/fixtures.py --program $(FIXTURE_PROBE) tests/fixtures/foundation

test-lexer: $(LEXER_TEST)
	$(LEXER_TEST)

test-lexer-fixtures: $(LEXER_PROBE)
	$(PYTHON) scripts/testing/fixtures.py --program $(LEXER_PROBE) tests/fixtures/lexer

test-parser: $(PARSER_TEST)
	$(PARSER_TEST)

test-parser-fixtures: $(PARSER_PROBE)
	$(PYTHON) scripts/testing/fixtures.py --program $(PARSER_PROBE) tests/fixtures/parser

test-semantic: $(SEMANTIC_TEST)
	$(SEMANTIC_TEST)

test-semantic-fixtures: $(SEMANTIC_PROBE)
	$(PYTHON) scripts/testing/fixtures.py --program $(SEMANTIC_PROBE) tests/fixtures/semantic

test-runtime: $(RUNTIME_TEST)
	$(RUNTIME_TEST)

test-runtime-fixtures: $(RUNTIME_PROBE)
	$(PYTHON) scripts/testing/fixtures.py --program $(RUNTIME_PROBE) tests/fixtures/runtime

test-backend: $(BACKEND_TEST)
	$(BACKEND_TEST)

test-native: $(COMPILER)
	RUVA_TEST_COMPILER="$(abspath $(COMPILER))" RUVA_TEST_SANITIZE=$(NATIVE_SANITIZE) \
	    $(PYTHON) scripts/testing/fixtures.py --program scripts/testing/native.py tests/fixtures/native

test-integration: $(COMPILER)
	RUVA_TEST_COMPILER="$(abspath $(COMPILER))" RUVA_TEST_SANITIZE=$(NATIVE_SANITIZE) \
	    $(PYTHON) -m unittest discover -s tests/integration -v

test-python:
	$(PYTHON) -m unittest discover -s tests/repository -v
	$(PYTHON) -m unittest discover -s tests/testing -v

ifneq ($(WINDOWS_TARGET),)
sanitize:
	@echo "Sanitizers are unavailable on the Windows GCC target." >&2
	@exit 1
else
sanitize:
	$(MAKE) BUILD_DIR=$(SANITIZE_BUILD_DIR) NATIVE_SANITIZE=1 \
	    CFLAGS="-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all" \
	    LDFLAGS="-fsanitize=address,undefined" \
	    test-foundation test-fixtures test-lexer test-lexer-fixtures test-parser test-parser-fixtures \
	    test-semantic test-semantic-fixtures test-runtime test-runtime-fixtures \
	    test-backend test-native test-integration test-host
endif

clean:
	rm -rf build

-include $(FOUNDATION_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d) $(PROBE_OBJECTS:.o=.d)
-include $(LEXER_OBJECTS:.o=.d) $(LEXER_TEST_OBJECTS:.o=.d) $(LEXER_PROBE_OBJECT:.o=.d)
-include $(PARSER_OBJECTS:.o=.d)
-include $(PARSER_TEST_OBJECTS:.o=.d) $(PARSER_PROBE_OBJECT:.o=.d)
-include $(ANALYSIS_OBJECTS:.o=.d) $(SEMANTIC_PROBE_OBJECT:.o=.d)
-include $(SEMANTIC_TEST_OBJECTS:.o=.d)

-include $(RUNTIME_OBJECTS:.o=.d) $(BACKEND_OBJECTS:.o=.d) $(DRIVER_OBJECTS:.o=.d)
-include $(RUNTIME_TEST_OBJECTS:.o=.d) $(RUNTIME_PROBE_OBJECT:.o=.d) $(BACKEND_TEST_OBJECTS:.o=.d)
-include $(HOST_TEST_OBJECTS:.o=.d)
