#include "../ruva_runtime.h"

int32_t rv_array_length(RvRuntime *runtime, const RvArray *array, RvLocation where) {
    if (array->length > INT32_MAX) {
        rv_panic(runtime, "array length exceeds int range", where);
    }
    return (int32_t)array->length;
}

void rv_array_check(RvRuntime *runtime, const RvArray *array, int32_t index, RvLocation where) {
    if (index < 0 || (size_t)index >= array->length) {
        rv_panic(runtime, "array index out of bounds", where);
    }
}

void rv_array_push(RvRuntime *runtime, RvArray *array, RvElement value, RvLocation where) {
    if (array->length >= INT32_MAX) {
        rv_panic(runtime, "array length exceeds int range", where);
    }
    if (array->length == array->capacity) {
        size_t capacity = array->capacity == 0 ? 4 : array->capacity;
        if (capacity <= (size_t)INT32_MAX / 2) {
            capacity *= 2;
        } else {
            capacity = INT32_MAX;
        }
        if (capacity > SIZE_MAX / sizeof(*array->data)) {
            rv_panic(runtime, "array capacity overflow", where);
        }
        RvElement *data = rv_try_resize(runtime, array->data, capacity * sizeof(*data));
        if (data == NULL) {
            rv_panic(runtime, "out of memory growing array", where);
        }
        array->data = data;
        array->capacity = capacity;
    }
    array->data[array->length++] = value;
}

RvElement rv_array_pop(RvRuntime *runtime, RvArray *array, RvLocation where) {
    if (array->length == 0) {
        rv_panic(runtime, "cannot pop an empty array", where);
    }
    return array->data[--array->length];
}

RvElement rv_array_get(RvRuntime *runtime, const RvArray *array, int32_t index, RvLocation where) {
    rv_array_check(runtime, array, index, where);
    RvElement value = array->data[index];
    if (array->owns_text) {
        value.text = rv_text_retain(runtime, value.text, where);
    }
    return value;
}

void rv_array_set(RvRuntime *runtime, RvArray *array, int32_t index,
                  RvElement value, RvLocation where) {
    rv_array_check(runtime, array, index, where);
    if (array->owns_text) {
        rv_text_release(runtime, array->data[index].text);
    }
    array->data[index] = value;
}

void rv_array_release(RvRuntime *runtime, RvArray *array) {
    if (array->owns_text) {
        for (size_t index = 0; index < array->length; ++index) {
            rv_text_release(runtime, array->data[index].text);
        }
    }
    rv_release(runtime, array->data);
    *array = (RvArray){0};
}
