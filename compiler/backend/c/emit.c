#define _XOPEN_SOURCE 700

#include "internal.h"

#include <stdarg.h>
#include <string.h>

bool rv_c_fail(RvEmitError *error, RvEmitErrorKind kind, RvSpan span, const char *message) {
    if (error->kind == RV_EMIT_OK) {
        *error = (RvEmitError){kind, span, message};
    }
    return false;
}

bool rv_c_write(RvCEmitter *emitter, const char *format, ...) {
    if (emitter->error->kind != RV_EMIT_OK) {
        return false;
    }
    va_list arguments;
    va_start(arguments, format);
    int written = vfprintf(emitter->output, format, arguments);
    va_end(arguments);
    if (written < 0) {
        return rv_c_fail(emitter->error, RV_EMIT_IO, (RvSpan){0}, "cannot write generated C");
    }
    const size_t limit = 16 * 1024 * 1024;
    if ((size_t)written > limit - emitter->generated_bytes) {
        return rv_c_fail(emitter->error, RV_EMIT_LIMIT, emitter->current_span,
                         "generated C exceeds the 16 MiB limit");
    }
    emitter->generated_bytes += (size_t)written;
    return true;
}

void rv_c_indent(RvCEmitter *emitter) {
    for (size_t index = 0; index < emitter->indentation; ++index) {
        rv_c_write(emitter, "    ");
    }
}

void *rv_c_new(RvCEmitter *emitter, size_t size, RvSpan span) {
    if (emitter->error->kind != RV_EMIT_OK) {
        return NULL;
    }
    void *node = rv_arena_allocate(&emitter->storage, size);
    if (node == NULL) {
        rv_c_fail(emitter->error, RV_EMIT_ALLOCATION, span, "out of memory while emitting C");
    } else {
        memset(node, 0, size);
    }
    return node;
}

bool rv_c_emit(FILE *output, const RvCheckedProgram *program,
                RvAllocator allocator, RvEmitError *error) {
    RvEmitError local_error;
    RvEmitError *result = error == NULL ? &local_error : error;
    if (!rv_c_supported(program, result)) {
        return false;
    }
    if (output == NULL) {
        return rv_c_fail(result, RV_EMIT_ARGUMENT, (RvSpan){0}, "C output stream is required");
    }
    RvCEmitter emitter = {
        .output = output, .error = result,
        .current_span = program->graph.entry->ast.span
    };
    rv_arena_init(&emitter.storage, allocator, 4096);
    rv_c_write(&emitter, "#include \"ruva_runtime.h\"\n\n");
    rv_c_records(&emitter, program);
    rv_c_functions(&emitter, program);
    emitter.function = program->graph.entry->ast.launch->symbol;
    rv_c_write(&emitter, "static void rv_launch(RvRuntime *rt)\n");
    rv_c_block(&emitter, program->graph.entry->ast.launch->as.function.body);
    rv_c_write(&emitter,
        "\nint main(void) {\n"
        "    RvRuntime runtime;\n"
        "    rv_runtime_init(&runtime, stdout, stderr);\n");
    rv_c_function_references(&emitter, program);
    rv_c_write(&emitter,
        "    rv_launch(&runtime);\n"
        "    return rv_runtime_finish(&runtime) ? 0 : 1;\n"
        "}\n");
    if (ferror(output)) {
        rv_c_fail(result, RV_EMIT_IO, (RvSpan){0}, "cannot write generated C");
    }
    rv_arena_destroy(&emitter.storage);
    return result->kind == RV_EMIT_OK;
}
