#include "module.h"
#include "path.h"
#include "compiler/common/host/files.h"
#include "compiler/frontend/parser/parser.h"

#include <errno.h>
#include <string.h>

typedef struct {
    RvModuleGraph *graph;
    RvAllocator allocator;
    RvCheckError *error;
} ModuleLoader;

static bool fail(ModuleLoader *loader, RvCheckErrorKind kind, RvSpan span,
                  const char *path, const char *message, int system_error) {
    *loader->error = (RvCheckError){kind, span, path, message, system_error};
    return false;
}

static void *new_item(ModuleLoader *loader, size_t size, RvSpan span) {
    void *item = rv_arena_allocate(&loader->graph->storage, size);
    if (item == NULL) {
        fail(loader, RV_CHECK_ALLOCATION, span, loader->graph->input_path,
             "out of memory while loading modules", 0);
    } else {
        memset(item, 0, size);
    }
    return item;
}

static bool parse_file(ModuleLoader *loader, RvModuleUnit *unit, bool entry, RvSpan import_site) {
    RvSourceError source_error;
    unit->source = rv_source_load(loader->allocator, unit->path, &source_error);
    if (unit->source == NULL) {
        RvCheckErrorKind kind = source_error.kind == RV_SOURCE_ALLOCATION
            ? RV_CHECK_ALLOCATION : RV_CHECK_IO;
        if (source_error.kind == RV_SOURCE_TOO_LARGE) {
            kind = RV_CHECK_LIMIT;
        }
        return fail(loader, kind, import_site, unit->path,
                    "cannot load source", source_error.system_error);
    }

    RvTokenStream tokens;
    RvLexError lex_error;
    if (!rv_lex_source(unit->source, loader->allocator, &tokens, &lex_error)) {
        RvCheckErrorKind kind = lex_error.kind == RV_LEX_ALLOCATION
            ? RV_CHECK_ALLOCATION : RV_CHECK_LEXICAL;
        return fail(loader, kind, lex_error.span, unit->path, lex_error.message, 0);
    }

    unit->token_count = rv_token_stream_count(&tokens);
    RvParseError parse_error;
    bool success = rv_parse_module(&tokens, entry, loader->allocator, &unit->ast, &parse_error);
    rv_token_stream_destroy(&tokens);

    if (!success) {
        RvCheckErrorKind kind = RV_CHECK_SYNTAX;
        if (parse_error.kind == RV_PARSE_ALLOCATION) {
            kind = RV_CHECK_ALLOCATION;
        } else if (parse_error.kind == RV_PARSE_LIMIT) {
            kind = RV_CHECK_LIMIT;
        }
        return fail(loader, kind, parse_error.span, unit->path, parse_error.message, 0);
    }
    return true;
}

static RvModuleUnit *load_unit(ModuleLoader *loader, const char *path, bool entry,
                                RvSpan import_site, size_t depth) {
    if (depth >= 128) {
        fail(loader, RV_CHECK_LIMIT, import_site, path, "module nesting limit exceeded", 0);
        return NULL;
    }

    int system_error;
    char *canonical = rv_module_canonical_path(&loader->graph->storage, path, &system_error);
    if (canonical == NULL) {
        RvCheckErrorKind kind = system_error == ENOMEM ? RV_CHECK_ALLOCATION : RV_CHECK_IO;
        fail(loader, kind, import_site, path, "cannot resolve source path", system_error);
        return NULL;
    }

    for (size_t index = 0; index < loader->graph->modules.length; ++index) {
        RvModuleUnit *cached = rv_module_graph_at(loader->graph, index);
        bool same = strcmp(cached->path, canonical) == 0;
#ifdef _WIN32
        same = same || rv_host_same_file(cached->path, canonical);
#endif
        if (same) {
            if (cached->state == 1) {
                fail(loader, RV_CHECK_IMPORT, import_site, canonical, "circular import", 0);
                return NULL;
            }
            return cached;
        }
    }

    RvModuleUnit *unit = new_item(loader, sizeof(*unit), import_site);
    if (unit == NULL) {
        return NULL;
    }
    RvModuleUnit **slot = rv_vector_append(&loader->graph->modules);
    if (slot == NULL) {
        fail(loader, RV_CHECK_ALLOCATION, import_site, canonical,
             "out of memory while loading modules", 0);
        return NULL;
    }

    *slot = unit;
    unit->id = loader->graph->modules.length;
    unit->path = canonical;
    unit->state = 1;
    rv_arena_init(&unit->ast.storage, loader->allocator, 4096);
    if (!parse_file(loader, unit, entry, import_site)) {
        return NULL;
    }

    RvImportEdge **tail = &unit->imports;
    for (const RvAstImport *import = unit->ast.imports; import != NULL; import = import->next) {
        const char *relative = import->path->text;
        if (!rv_module_relative_path(relative)) {
            fail(loader, RV_CHECK_IMPORT, import->path->span, unit->path,
                 "import path must be nonempty and relative", 0);
            return NULL;
        }

        char *requested = rv_module_import_path(&loader->graph->storage, unit->path, relative);
        if (requested == NULL) {
            fail(loader, RV_CHECK_ALLOCATION, import->path->span, unit->path,
                 "out of memory while resolving import", 0);
            return NULL;
        }

        RvModuleUnit *dependency = load_unit(loader, requested, false, import->path->span, depth + 1);
        if (dependency == NULL) {
            return NULL;
        }
        RvImportEdge *edge = new_item(loader, sizeof(*edge), import->path->span);
        if (edge == NULL) {
            return NULL;
        }
        edge->syntax = import;
        edge->target = dependency;
        *tail = edge;
        tail = &edge->next;
    }

    unit->state = 2;
    return unit;
}

bool rv_module_graph_load(const char *path, RvAllocator allocator,
                           RvModuleGraph *output, RvCheckError *error) {
    RvCheckError local_error = {0};
    RvCheckError *result = error == NULL ? &local_error : error;
    *result = (RvCheckError){0};
    if (output == NULL) {
        *result = (RvCheckError){.kind = RV_CHECK_ARGUMENT, .message = "module output is required"};
        return false;
    }

    *output = (RvModuleGraph){0};
    rv_arena_init(&output->storage, allocator, 4096);
    rv_vector_init(&output->modules, allocator, sizeof(RvModuleUnit *));
    if (path == NULL || path[0] == '\0') {
        *result = (RvCheckError){.kind = RV_CHECK_ARGUMENT, .message = "entry path is required"};
        return false;
    }

    output->input_path = rv_arena_copy(&output->storage, path, strlen(path));
    if (output->input_path == NULL) {
        *result = (RvCheckError){.kind = RV_CHECK_ALLOCATION,
                               .message = "out of memory while loading modules"};
        return false;
    }

    ModuleLoader loader = {output, allocator, result};
    output->entry = load_unit(&loader, output->input_path, true, (RvSpan){0}, 0);
    return output->entry != NULL;
}

void rv_module_graph_destroy(RvModuleGraph *graph) {
    for (size_t index = 0; index < graph->modules.length; ++index) {
        RvModuleUnit *unit = rv_module_graph_at(graph, index);
        rv_ast_module_destroy(&unit->ast);
        rv_source_destroy(unit->source);
    }
    rv_vector_destroy(&graph->modules);
    rv_arena_destroy(&graph->storage);
    graph->entry = NULL;
    graph->input_path = NULL;
}

RvModuleUnit *rv_module_graph_at(const RvModuleGraph *graph, size_t index) {
    RvModuleUnit **slot = rv_vector_at(&graph->modules, index);
    return slot == NULL ? NULL : *slot;
}
