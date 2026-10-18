CC ?= cc
AR ?= ar
PYTHON ?= python3
BUILD_DIR ?= build/debug
CPPFLAGS ?=
CFLAGS ?= -O0 -g
LDFLAGS ?=
LDLIBS ?=
WARNINGS = -std=c11 -Wall -Wextra -Wpedantic -Werror

FOUNDATION_SOURCES = compiler/common/memory/allocator.c \
    compiler/common/memory/arena.c compiler/common/memory/vector.c \
    compiler/common/source/source.c compiler/common/diagnostics/diagnostic.c
TEST_SOURCES = tests/foundation/main.c tests/foundation/support.c \
    tests/foundation/memory.c tests/foundation/source.c
PROBE_SOURCES = tests/foundation/probe.c tests/foundation/support.c
FOUNDATION_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(FOUNDATION_SOURCES))
TEST_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SOURCES))
PROBE_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(PROBE_SOURCES))
LIBRARY = $(BUILD_DIR)/lib/libruva_foundation.a
FOUNDATION_TEST = $(BUILD_DIR)/tests/foundation/foundation-tests
FIXTURE_PROBE = $(BUILD_DIR)/tests/foundation/fixture-probe

.PHONY: all test test-foundation test-fixtures test-python check-repository sanitize clean

all: $(LIBRARY) $(FOUNDATION_TEST) $(FIXTURE_PROBE)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -I. -MMD -MP -c $< -o $@

$(LIBRARY): $(FOUNDATION_OBJECTS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(FOUNDATION_TEST): $(TEST_OBJECTS) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(FIXTURE_PROBE): $(PROBE_OBJECTS) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test: check-repository test-foundation test-fixtures test-python

check-repository:
	$(PYTHON) scripts/repository/check-layout.py
	$(PYTHON) scripts/repository/check-docs.py

test-foundation: $(FOUNDATION_TEST)
	$(FOUNDATION_TEST)

test-fixtures: $(FIXTURE_PROBE)
	$(PYTHON) scripts/testing/fixtures.py --program $(FIXTURE_PROBE) tests/fixtures/foundation

test-python:
	$(PYTHON) -m unittest discover -s tests/repository -v
	$(PYTHON) -m unittest discover -s tests/testing -v

sanitize:
	$(MAKE) BUILD_DIR=build/sanitize \
	    CFLAGS="-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all" \
	    LDFLAGS="-fsanitize=address,undefined" test-foundation test-fixtures

clean:
	rm -rf build

-include $(FOUNDATION_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d) $(PROBE_OBJECTS:.o=.d)
