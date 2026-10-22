#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE

#include "../test.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

bool test_project_init(TestProject *project) {
    strcpy(project->directory, "/tmp/ruva-semantic-XXXXXX");
    CHECK(mkdtemp(project->directory) != NULL);
    CHECK(snprintf(project->entry, sizeof(project->entry), "%s/main.ruv",
                   project->directory) > 0);
    return true;
}

bool test_project_write(const TestProject *project, const char *name, const char *source) {
    char path[512];
    int length = snprintf(path, sizeof(path), "%s/%s", project->directory, name);
    CHECK(length > 0 && (size_t)length < sizeof(path));
    for (char *part = path + strlen(project->directory) + 1; *part; ++part) {
        if (*part == '/') {
            *part = '\0';
            CHECK(mkdir(path, 0700) == 0 || errno == EEXIST);
            *part = '/';
        }
    }
    FILE *stream = fopen(path, "wb");
    CHECK(stream != NULL);
    CHECK(fwrite(source, 1, strlen(source), stream) == strlen(source));
    CHECK(fclose(stream) == 0);
    return true;
}

static bool remove_directory(const char *path) {
    DIR *directory = opendir(path);
    CHECK(directory != NULL);
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char child[1024];
        int length = snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
        CHECK(length > 0 && (size_t)length < sizeof(child));
        struct stat info;
        CHECK(lstat(child, &info) == 0);
        if (S_ISDIR(info.st_mode)) {
            CHECK(remove_directory(child));
        } else {
            CHECK(unlink(child) == 0);
        }
    }
    CHECK(closedir(directory) == 0);
    CHECK(rmdir(path) == 0);
    return true;
}

bool test_project_remove(TestProject *project) {
    return remove_directory(project->directory);
}

bool test_accept(const char *source, RvCheckedProgram *program) {
    TestProject project;
    CHECK(test_project_init(&project));
    CHECK(test_project_write(&project, "main.ruv", source));
    RvCheckError error;
    bool success = rv_check_program(project.entry, rv_allocator_system(), program, &error);
    if (!success) {
        rv_check_error_write(stderr, &error);
    }
    CHECK(test_project_remove(&project));
    CHECK(success && program->valid && error.kind == RV_CHECK_OK);
    return true;
}

bool test_semantic_cases(const SemanticCase *cases, size_t count) {
    for (size_t index = 0; index < count; ++index) {
        const SemanticCase *item = &cases[index];
        TestProject project;
        CHECK(test_project_init(&project));
        CHECK(test_project_write(&project, "main.ruv", item->source));
        TestAllocator allocation = {0};
        RvCheckedProgram program;
        RvCheckError error;
        bool success = rv_check_program(project.entry, test_allocator(&allocation), &program, &error);
        bool matches = success == (item->kind == RV_CHECK_OK) && error.kind == item->kind;
        if (item->message != NULL) {
            matches = matches && error.message != NULL && strstr(error.message, item->message) != NULL;
        }
        if (!matches) {
            fprintf(stderr, "semantic case %zu: %s\n", index, item->source);
            rv_check_error_write(stderr, &error);
        }
        if (!success) {
            CHECK(!program.valid);
            if (error.span.source != NULL) {
                CHECK(error.span.start <= error.span.end);
                CHECK(error.span.end <= rv_source_length(error.span.source));
            }
            FILE *diagnostic = tmpfile();
            CHECK(diagnostic != NULL);
            CHECK(rv_check_error_write(diagnostic, &error));
            CHECK(fclose(diagnostic) == 0);
        }
        rv_checked_program_destroy(&program);
        rv_checked_program_destroy(&program);
        CHECK(allocation.live == 0);
        CHECK(test_project_remove(&project));
        CHECK(matches);
    }
    return true;
}

RvAstStmt *test_statement(RvCheckedProgram *program, size_t index) {
    RvAstSequence *sequence = program->graph.entry->ast.launch->as.function.body->as.statements;
    while (sequence != NULL && index-- != 0) {
        sequence = sequence->next;
    }
    return sequence == NULL ? NULL : sequence->statement;
}

RvSymbol *test_symbol(RvCheckedProgram *program, const char *name, RvSymbolKind kind) {
    for (size_t index = 0; index < program->symbols.length; ++index) {
        RvSymbol *symbol = rv_checked_symbol_at(program, index);
        if (symbol->kind == kind && strcmp(symbol->name, name) == 0) {
            return symbol;
        }
    }
    return NULL;
}
