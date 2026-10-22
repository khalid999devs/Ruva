CC ?= cc
AR ?= ar
PYTHON ?= python3
FLEX ?= flex
BUILD_DIR ?= build/debug
CPPFLAGS ?=
CFLAGS ?= -O0 -g
LDFLAGS ?=
LDLIBS ?=
WARNINGS = -std=c11 -Wall -Wextra -Wpedantic -Werror

HOST_SOURCES = compiler/common/host/files.c compiler/common/host/numeric.c
ifneq ($(findstring mingw,$(shell $(CC) -dumpmachine)),)
HOST_SOURCES += compiler/common/host/win32/utf8.c
endif

FOUNDATION_SOURCES = compiler/common/memory/allocator.c \
    compiler/common/memory/arena.c compiler/common/memory/vector.c \
    compiler/common/source/source.c compiler/common/diagnostics/diagnostic.c \
    compiler/common/host/streams.c $(HOST_SOURCES)
TEST_SOURCES = tests/foundation/main.c tests/foundation/support.c \
    tests/foundation/memory.c tests/foundation/source.c
PROBE_SOURCES = tests/foundation/probe.c tests/foundation/support.c
FOUNDATION_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(FOUNDATION_SOURCES))
TEST_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SOURCES))
PROBE_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(PROBE_SOURCES))
LIBRARY = $(BUILD_DIR)/lib/libruva_foundation.a
FOUNDATION_TEST = $(BUILD_DIR)/tests/foundation/foundation-tests
FIXTURE_PROBE = $(BUILD_DIR)/tests/foundation/fixture-probe

LEXER_SOURCES = compiler/frontend/lexer/token.c compiler/frontend/lexer/lexer.c
LEXER_TEST_SOURCES = tests/lexer/main.c tests/lexer/tokens.c \
    tests/lexer/text.c tests/lexer/failure.c tests/foundation/support.c
LEXER_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(LEXER_SOURCES)) \
    $(BUILD_DIR)/compiler/frontend/lexer/scanner.o
LEXER_TEST_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(LEXER_TEST_SOURCES))
LEXER_PROBE_OBJECT = $(BUILD_DIR)/tests/lexer/probe.o
LEXER_LIBRARY = $(BUILD_DIR)/lib/libruva_lexer.a
LEXER_TEST = $(BUILD_DIR)/tests/lexer/lexer-tests
LEXER_PROBE = $(BUILD_DIR)/tests/lexer/lexer-probe
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
PARSER_TEST = $(BUILD_DIR)/tests/parser/parser-tests
PARSER_PROBE_OBJECT = $(BUILD_DIR)/tests/parser/tools/probe.o
PARSER_PROBE = $(BUILD_DIR)/tests/parser/parser-probe

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
SEMANTIC_PROBE = $(BUILD_DIR)/tests/semantic/semantic-probe

.PHONY: all test test-foundation test-fixtures test-lexer test-lexer-fixtures test-parser \
    test-parser-fixtures test-semantic test-semantic-fixtures test-python check-repository sanitize clean

all: $(LIBRARY) $(FOUNDATION_TEST) $(FIXTURE_PROBE) $(LEXER_LIBRARY) $(LEXER_TEST) $(LEXER_PROBE) \
    $(PARSER_LIBRARY) $(PARSER_TEST) $(PARSER_PROBE) \
    $(ANALYSIS_LIBRARY) $(SEMANTIC_TEST) $(SEMANTIC_PROBE)

$(LIBRARY) $(LEXER_LIBRARY) $(PARSER_LIBRARY) $(ANALYSIS_LIBRARY): Makefile

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

$(LEXER_TEST): $(LEXER_TEST_OBJECTS) $(LEXER_LIBRARY) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(LEXER_PROBE): $(LEXER_PROBE_OBJECT) $(LEXER_LIBRARY) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(PARSER_LIBRARY): $(PARSER_OBJECTS)
	@mkdir -p $(dir $@)
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(PARSER_TEST): $(PARSER_TEST_OBJECTS) $(PARSER_LIBRARY) $(LEXER_LIBRARY) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(PARSER_PROBE): $(PARSER_PROBE_OBJECT) $(PARSER_LIBRARY) $(LEXER_LIBRARY) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(ANALYSIS_LIBRARY): $(ANALYSIS_OBJECTS)
	@mkdir -p $(dir $@)
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(SEMANTIC_PROBE): $(SEMANTIC_PROBE_OBJECT) $(ANALYSIS_LIBRARY) $(PARSER_LIBRARY) $(LEXER_LIBRARY) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(SEMANTIC_TEST): $(SEMANTIC_TEST_OBJECTS) $(ANALYSIS_LIBRARY) $(PARSER_LIBRARY) $(LEXER_LIBRARY) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test: check-repository test-foundation test-fixtures test-lexer test-lexer-fixtures test-parser \
    test-parser-fixtures test-semantic test-semantic-fixtures test-python

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

test-python:
	$(PYTHON) -m unittest discover -s tests/repository -v
	$(PYTHON) -m unittest discover -s tests/testing -v

sanitize:
	$(MAKE) BUILD_DIR=build/sanitize \
	    CFLAGS="-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all" \
	    LDFLAGS="-fsanitize=address,undefined" test-foundation test-fixtures test-lexer \
	    test-lexer-fixtures test-parser test-parser-fixtures \
	    test-semantic test-semantic-fixtures

clean:
	rm -rf build

-include $(FOUNDATION_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d) $(PROBE_OBJECTS:.o=.d)
-include $(LEXER_OBJECTS:.o=.d) $(LEXER_TEST_OBJECTS:.o=.d) $(LEXER_PROBE_OBJECT:.o=.d)
-include $(PARSER_OBJECTS:.o=.d)
-include $(PARSER_TEST_OBJECTS:.o=.d) $(PARSER_PROBE_OBJECT:.o=.d)
-include $(ANALYSIS_OBJECTS:.o=.d) $(SEMANTIC_PROBE_OBJECT:.o=.d)
-include $(SEMANTIC_TEST_OBJECTS:.o=.d)
