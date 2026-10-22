#include "internal.h"

#include <string.h>

bool rv_check_fail(RvChecker *checker, RvCheckErrorKind kind, RvSpan span, const char *message) {
    if (checker->error->kind == RV_CHECK_OK) {
        const char *path = checker->unit == NULL ? checker->program->graph.input_path : checker->unit->path;
        *checker->error = (RvCheckError){kind, span, path, message, 0};
    }
    return false;
}

bool rv_check_step(RvChecker *checker, RvSpan span) {
    if (checker->error->kind != RV_CHECK_OK) {
        return false;
    }
    if (++checker->steps > checker->step_limit) {
        return rv_check_fail(checker, RV_CHECK_LIMIT, span, "semantic work limit exceeded");
    }
    return true;
}

void *rv_check_new(RvChecker *checker, size_t size, RvSpan span) {
    if (!rv_check_step(checker, span)) {
        return NULL;
    }
    void *object = rv_arena_allocate(&checker->program->storage, size);
    if (object == NULL) {
        rv_check_fail(checker, RV_CHECK_ALLOCATION, span, "out of memory while checking source");
        return NULL;
    }
    memset(object, 0, size);
    return object;
}

bool rv_check_program(const char *path, RvAllocator allocator,
                       RvCheckedProgram *output, RvCheckError *error) {
    RvCheckError local_error = {0};
    RvCheckError *result = error == NULL ? &local_error : error;
    *result = (RvCheckError){0};
    if (output == NULL) {
        *result = (RvCheckError){.kind = RV_CHECK_ARGUMENT, .message = "checked output is required"};
        return false;
    }

    *output = (RvCheckedProgram){0};
    rv_arena_init(&output->storage, allocator, 4096);
    rv_vector_init(&output->symbols, allocator, sizeof(RvSymbol *));
    if (!rv_module_graph_load(path, allocator, &output->graph, result)) {
        return false;
    }

    RvChecker checker = {.program = output, .error = result, .step_limit = 4096};
    for (size_t index = 0; index < output->graph.modules.length; ++index) {
        RvModuleUnit *unit = rv_module_graph_at(&output->graph, index);
        size_t budget;
        if (!rv_size_multiply(unit->token_count, 128, &budget)
            || !rv_size_add(checker.step_limit, budget, &checker.step_limit)) {
            return rv_check_fail(&checker, RV_CHECK_LIMIT, unit->ast.span, "program is too large");
        }
    }

    output->valid = rv_check_builtins(&checker)
        && rv_check_declarations(&checker)
        && rv_check_bodies(&checker)
        && result->kind == RV_CHECK_OK;
    return output->valid;
}

void rv_checked_program_destroy(RvCheckedProgram *program) {
    rv_module_graph_destroy(&program->graph);
    rv_vector_destroy(&program->symbols);
    rv_arena_destroy(&program->storage);
    program->valid = false;
}

RvSymbol *rv_checked_symbol_at(const RvCheckedProgram *program, size_t index) {
    RvSymbol **slot = rv_vector_at(&program->symbols, index);
    return slot == NULL ? NULL : *slot;
}
