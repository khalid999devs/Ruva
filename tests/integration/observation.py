import os
import subprocess

from support import ROOT, RUNTIME_SOURCES, SANITIZE, CompilerTestCase


OBSERVE_ALLOCATIONS = r'''
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

static RvRuntimeAllocator original_allocator;
static size_t observed_live;
static size_t observed_peak;
static size_t observed_calls;
static size_t observed_fail_at;
static bool observed_watch_files;
static bool initial_fds[256];

static void capture_files(void) {
    if (observed_watch_files) {
        for (int descriptor = 0; descriptor < 256; ++descriptor) {
            initial_fds[descriptor] = fcntl(descriptor, F_GETFD) != -1;
        }
    }
}

typedef union {
    max_align_t alignment;
    size_t size;
} ObservedBlock;

static void *observe_allocate(void *context, size_t size) {
    (void)context;
    if (++observed_calls == observed_fail_at || size > SIZE_MAX - sizeof(ObservedBlock)) {
        return NULL;
    }
    ObservedBlock *block = original_allocator.allocate(original_allocator.context,
                                                       sizeof(*block) + size);
    if (block == NULL) {
        return NULL;
    }
    block->size = size;
    ++observed_live;
    if (observed_live > observed_peak) {
        observed_peak = observed_live;
    }
    return block + 1;
}

static void *observe_resize(void *context, void *pointer, size_t size) {
    (void)context;
    if (++observed_calls == observed_fail_at || size > SIZE_MAX - sizeof(ObservedBlock)) {
        return NULL;
    }
    ObservedBlock *block = (ObservedBlock *)pointer - 1;
    ObservedBlock *replacement = original_allocator.allocate(original_allocator.context,
                                                             sizeof(*block) + size);
    if (replacement == NULL) {
        return NULL;
    }
    replacement->size = size;
    memcpy(replacement + 1, pointer, block->size < size ? block->size : size);
    original_allocator.release(original_allocator.context, block);
    return replacement + 1;
}

static void observe_release(void *context, void *pointer) {
    (void)context;
    --observed_live;
    original_allocator.release(original_allocator.context, (ObservedBlock *)pointer - 1);
}

static void verify_fatal_cleanup(void) {
    if (observed_watch_files) {
        for (int descriptor = 0; descriptor < 256; ++descriptor) {
            if ((fcntl(descriptor, F_GETFD) != -1) != initial_fds[descriptor]) {
                _Exit(96);
            }
        }
    }
    if (observed_live != 0) {
        _Exit(92);
    }
}
'''


class ObservedCompilerTestCase(CompilerTestCase):
    def run_observed(self, source, peak_limit=0, persistent_files=0,
                     input_data=b"", watch_files=False):
        self.source.write_text(source)
        result = self.invoke("--emit-c", "input.ruv", "-o", "observed.c")
        self.assertEqual(result.returncode, 0, result.stderr)
        path = self.directory / "observed.c"
        code = path.read_text().replace(
            '#include "ruva_runtime.h"', '#include "ruva_runtime.h"\n' + OBSERVE_ALLOCATIONS
        )
        code = code.replace(
            "rv_runtime_init(&runtime, stdout, stderr);",
            "rv_runtime_init(&runtime, stdout, stderr);\n"
            "    original_allocator = runtime.allocator;\n"
            f"    observed_watch_files = {str(watch_files).lower()};\n"
            "    capture_files();\n"
            "    const char *failure = getenv(\"RV_FAIL_AT\");\n"
            "    observed_fail_at = failure == NULL ? 0 : strtoull(failure, NULL, 10);\n"
            "    runtime.allocator = (RvRuntimeAllocator){NULL, observe_allocate, "
            "observe_resize, observe_release};\n"
            "    if (atexit(verify_fatal_cleanup) != 0) return 94;"
        )
        checks = (f"\n    if (runtime.live_allocations != {persistent_files} "
                  f"|| observed_live != {persistent_files}) return 91;"
                  "\n    FILE *counts = fopen(\"allocation-count.txt\", \"w\");"
                  "\n    if (counts == NULL) return 95;"
                  "\n    fprintf(counts, \"%zu\\n\", observed_calls);"
                  "\n    if (fclose(counts) != 0) return 95;")
        if peak_limit:
            checks += f"\n    if (observed_peak > {peak_limit}) return 93;"
        code = code.replace("rv_launch(&runtime);", "rv_launch(&runtime);" + checks)
        path.write_text(code)
        arguments = [os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
                     "-Wpedantic", "-Werror", "-I", str(ROOT / "runtime"), "observed.c"]
        arguments.extend(str(ROOT / "runtime" / name) for name in RUNTIME_SOURCES)
        arguments.extend(["-o", "observed"])
        if SANITIZE:
            arguments.extend(["-fsanitize=address,undefined", "-fno-sanitize-recover=all"])
        result = subprocess.run(arguments, cwd=self.directory, capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        return self.run_observed_executable(input_data=input_data)

    def run_observed_executable(self, fail_at=0, input_data=b""):
        environment = self.environment.copy()
        environment["RV_FAIL_AT"] = str(fail_at)
        return subprocess.run([str(self.directory / "observed")], cwd=self.directory,
                              env=environment, input=input_data, capture_output=True, timeout=15)
