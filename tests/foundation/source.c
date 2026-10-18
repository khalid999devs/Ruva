#include "test.h"
#include "compiler/common/source/source.h"
#include "compiler/common/diagnostics/diagnostic.h"

#include <string.h>

static bool owned_source_and_locations(void) {
    char path[] = "lesson.ruv";
    char bytes[] = "a\r\n\tb\rc\n";
    const RvSourcePosition positions[] = {
        {1, 1}, {1, 2}, {1, 3}, {2, 1}, {2, 2},
        {2, 3}, {3, 1}, {3, 2}, {4, 1}
    };
    RvSourceError error;
    RvSource *source = rv_source_from_bytes(rv_allocator_system(), path,
                                           bytes, sizeof(bytes) - 1, &error);
    CHECK(source != NULL && error.kind == RV_SOURCE_OK);
    path[0] = 'x';
    bytes[0] = 'z';
    CHECK(strcmp(rv_source_path(source), "lesson.ruv") == 0);
    CHECK(rv_source_bytes(source)[0] == 'a');
    CHECK(rv_source_bytes(source)[rv_source_length(source)] == 0);
    CHECK(rv_source_line_count(source) == 4);
    for (size_t offset = 0; offset < sizeof(positions) / sizeof(positions[0]); ++offset) {
        RvSourcePosition actual;
        CHECK(rv_source_position(source, offset, &actual));
        CHECK(actual.line == positions[offset].line && actual.column == positions[offset].column);
    }
    RvSourceLine line;
    CHECK(rv_source_line(source, 2, &line));
    CHECK(line.length == 2 && memcmp(line.bytes, "\tb", 2) == 0);
    CHECK(rv_source_line(source, 4, &line) && line.length == 0);
    CHECK(!rv_source_line(source, 0, &line));
    CHECK(!rv_source_line(source, 5, &line));
    RvSourcePosition unchanged = {9, 9};
    CHECK(!rv_source_position(source, rv_source_length(source) + 1, &unchanged));
    CHECK(unchanged.line == 9 && unchanged.column == 9);
    rv_source_destroy(source);
    return true;
}

static bool empty_and_raw_sources(void) {
    RvSourceError error;
    RvSource *empty = rv_source_from_bytes(rv_allocator_system(), "empty.ruv", NULL, 0, &error);
    CHECK(empty != NULL && rv_source_length(empty) == 0 && rv_source_line_count(empty) == 1);
    RvSourcePosition position;
    CHECK(rv_source_position(empty, 0, &position) && position.line == 1 && position.column == 1);
    RvSourceLine line;
    CHECK(rv_source_line(empty, 1, &line) && line.length == 0 && line.bytes[0] == 0);
    rv_source_destroy(empty);

    const unsigned char bytes[] = {0xc3, 0xa9, 0, 'x'};
    RvSource *raw = rv_source_from_bytes(rv_allocator_system(), "raw.ruv", bytes, sizeof(bytes), NULL);
    CHECK(raw != NULL && memcmp(rv_source_bytes(raw), bytes, sizeof(bytes)) == 0);
    CHECK(rv_source_position(raw, 2, &position) && position.column == 3);
    rv_source_destroy(raw);
    CHECK(rv_source_from_bytes(rv_allocator_system(), NULL, "x", 1, &error) == NULL);
    CHECK(error.kind == RV_SOURCE_INVALID);
    CHECK(rv_source_from_bytes(rv_allocator_system(), "x", NULL, 1, &error) == NULL);
    CHECK(error.kind == RV_SOURCE_INVALID);
    CHECK(rv_source_from_bytes(rv_allocator_system(), "x", "x", SIZE_MAX, &error) == NULL);
    CHECK(error.kind == RV_SOURCE_TOO_LARGE);
    CHECK(rv_source_load(rv_allocator_system(), "", &error) == NULL && error.kind == RV_SOURCE_INVALID);
    rv_source_destroy(NULL);
    return true;
}

static bool source_allocation_failures(void) {
    char text[128];
    for (size_t index = 0; index < sizeof(text); ++index) {
        text[index] = index % 2 ? '\n' : 'x';
    }
    TestAllocator baseline = {0};
    RvSource *source = rv_source_from_bytes(test_allocator(&baseline), "many.ruv", text, sizeof(text), NULL);
    CHECK(source != NULL);
    size_t calls = baseline.calls;
    rv_source_destroy(source);
    CHECK(baseline.live == 0);
    for (size_t failure = 1; failure <= calls; ++failure) {
        TestAllocator state = {.fail_at = failure};
        RvSourceError error;
        source = rv_source_from_bytes(test_allocator(&state), "many.ruv", text, sizeof(text), &error);
        CHECK(source == NULL && error.kind == RV_SOURCE_ALLOCATION);
        CHECK(state.live == 0);
    }
    return true;
}

static bool spans_and_diagnostics(void) {
    RvSource *source = rv_source_from_bytes(rv_allocator_system(), "message.ruv", "a\nword", 6, NULL);
    CHECK(source != NULL);
    RvSpan span;
    CHECK(rv_span_create(&span, source, 3, 6));
    FILE *stream = tmpfile();
    CHECK(stream != NULL);
    CHECK(rv_diagnostic_write(stream, span, "expected 100% text"));
    CHECK(test_stream_equals(stream, "message.ruv:2:2: error: expected 100% text\n"));
    CHECK(!rv_span_create(&span, source, 4, 3));
    CHECK(!rv_span_create(&span, source, 0, 7));
    CHECK(!rv_diagnostic_write(stream, (RvSpan){source, 0, 7}, "invalid"));
    CHECK(!rv_diagnostic_write(stream, (RvSpan){0}, "invalid"));
    CHECK(fclose(stream) == 0);
    CHECK(rv_span_create(&span, source, 6, 6));
    rv_source_destroy(source);
    return true;
}

bool test_source(void) {
    return owned_source_and_locations() && empty_and_raw_sources()
        && source_allocation_failures() && spans_and_diagnostics();
}
