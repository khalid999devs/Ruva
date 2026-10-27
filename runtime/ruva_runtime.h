#ifndef RUVA_RUNTIME_H
#define RUVA_RUNTIME_H

#include "../compiler/common/platform.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
    const char *path;
    size_t line;
    size_t column;
} RvLocation;

typedef struct {
    void *context;
    void *(*allocate)(void *context, size_t size);
    void *(*resize)(void *context, void *pointer, size_t size);
    void (*release)(void *context, void *pointer);
} RvRuntimeAllocator;

typedef union RvAllocation RvAllocation;
typedef struct RvText RvText;
typedef struct RvFile RvFile;

typedef union {
    int32_t integer;
    double real;
    bool boolean;
    RvText *text;
} RvElement;

typedef struct {
    RvElement *data;
    size_t length;
    size_t capacity;
    bool owns_text;
} RvArray;

typedef struct {
    RvRuntimeAllocator allocator;
    RvAllocation *allocations;
    size_t live_allocations;
    size_t live_bytes;
    RvFile *files;
    FILE *input;
    FILE *output;
    FILE *errors;
    RvLocation output_location;
} RvRuntime;

void rv_runtime_init(RvRuntime *runtime, FILE *output, FILE *errors);
bool rv_runtime_cleanup(RvRuntime *runtime);
bool rv_runtime_finish(RvRuntime *runtime);
void rv_report_error(RvRuntime *runtime, const char *message, RvLocation where);
_Noreturn void rv_panic(RvRuntime *runtime, const char *message, RvLocation where);
void *rv_try_allocate(RvRuntime *runtime, size_t size);
void *rv_try_resize(RvRuntime *runtime, void *pointer, size_t size);
void *rv_allocate(RvRuntime *runtime, size_t size, RvLocation where);
void rv_release(RvRuntime *runtime, void *pointer);

int32_t rv_int_add(RvRuntime *runtime, int32_t left, int32_t right, RvLocation where);
int32_t rv_int_subtract(RvRuntime *runtime, int32_t left, int32_t right, RvLocation where);
int32_t rv_int_multiply(RvRuntime *runtime, int32_t left, int32_t right, RvLocation where);
int32_t rv_int_divide(RvRuntime *runtime, int32_t left, int32_t right, RvLocation where);
int32_t rv_int_remainder(RvRuntime *runtime, int32_t left, int32_t right, RvLocation where);
int32_t rv_int_negate(RvRuntime *runtime, int32_t value, RvLocation where);
double rv_real_divide(RvRuntime *runtime, double left, double right, RvLocation where);

RvText *rv_text_create(RvRuntime *runtime, const char *bytes, size_t length, RvLocation where);
RvText *rv_text_retain(RvRuntime *runtime, RvText *text, RvLocation where);
void rv_text_release(RvRuntime *runtime, RvText *text);
RvText *rv_text_concat(RvRuntime *runtime, const RvText *left, const RvText *right,
                       RvLocation where);
bool rv_text_equal(const RvText *left, const RvText *right);
bool rv_text_equal_bytes(const RvText *text, const char *bytes, size_t length);
const char *rv_text_bytes(const RvText *text);
size_t rv_text_length(const RvText *text);
size_t rv_format_real(RvRuntime *runtime, char *buffer, size_t capacity,
                       double value, RvLocation where);
RvText *rv_text_from_int(RvRuntime *runtime, int32_t value, RvLocation where);
RvText *rv_text_from_real(RvRuntime *runtime, double value, RvLocation where);
RvText *rv_text_from_bool(RvRuntime *runtime, bool value, RvLocation where);
int32_t rv_text_to_int(RvRuntime *runtime, const RvText *text, RvLocation where);
double rv_text_to_real(RvRuntime *runtime, const RvText *text, RvLocation where);
bool rv_text_to_bool(RvRuntime *runtime, const RvText *text, RvLocation where);
int32_t rv_real_to_int(RvRuntime *runtime, double value, RvLocation where);
double rv_int_to_real(RvRuntime *runtime, int32_t value, RvLocation where);

int32_t rv_ask_int(RvRuntime *runtime, RvLocation where);
double rv_ask_real(RvRuntime *runtime, RvLocation where);
bool rv_ask_bool(RvRuntime *runtime, RvLocation where);
RvText *rv_ask_text(RvRuntime *runtime, RvLocation where);

bool rv_file_write(RvRuntime *runtime, const RvText *path, const RvText *text, RvLocation where);
bool rv_file_append(RvRuntime *runtime, const RvText *path, const RvText *text, RvLocation where);
RvText *rv_file_read(RvRuntime *runtime, const RvText *path, RvLocation where);
bool rv_file_exists(RvRuntime *runtime, const RvText *path, RvLocation where);
RvFile *rv_file_open(RvRuntime *runtime, const RvText *path, const RvText *mode, RvLocation where);
RvText *rv_file_read_from(RvRuntime *runtime, RvFile *file, RvLocation where);
bool rv_file_write_to(RvRuntime *runtime, RvFile *file, const RvText *text, RvLocation where);
bool rv_file_close(RvRuntime *runtime, RvFile *file, RvLocation where);
bool rv_files_cleanup(RvRuntime *runtime);

int32_t rv_array_length(RvRuntime *runtime, const RvArray *array, RvLocation where);
void rv_array_check(RvRuntime *runtime, const RvArray *array, int32_t index, RvLocation where);
void rv_array_push(RvRuntime *runtime, RvArray *array, RvElement value, RvLocation where);
RvElement rv_array_pop(RvRuntime *runtime, RvArray *array, RvLocation where);
RvElement rv_array_get(RvRuntime *runtime, const RvArray *array, int32_t index, RvLocation where);
void rv_array_set(RvRuntime *runtime, RvArray *array, int32_t index,
                  RvElement value, RvLocation where);
void rv_array_release(RvRuntime *runtime, RvArray *array);

void rv_print_int(RvRuntime *runtime, int32_t value, RvLocation where);
void rv_print_real(RvRuntime *runtime, double value, RvLocation where);
void rv_print_bool(RvRuntime *runtime, bool value, RvLocation where);
void rv_print_text(RvRuntime *runtime, const RvText *value, RvLocation where);
void rv_print_separator(RvRuntime *runtime, RvLocation where);
void rv_print_newline(RvRuntime *runtime, RvLocation where);

#endif
