#include "vector.h"

bool rv_vector_init(RvVector *vector, RvAllocator allocator, size_t element_size) {
    *vector = (RvVector){allocator, NULL, 0, 0, element_size};
    return element_size != 0;
}

bool rv_vector_reserve(RvVector *vector, size_t minimum) {
    if (vector->element_size == 0) {
        return false;
    }
    if (minimum <= vector->capacity) {
        return true;
    }
    size_t maximum = SIZE_MAX / vector->element_size;
    if (minimum > maximum) {
        return false;
    }
    size_t capacity = vector->capacity ? vector->capacity : (maximum < 8 ? maximum : 8);
    while (capacity < minimum) {
        if (capacity > maximum / 2) {
            capacity = minimum;
            break;
        }
        capacity *= 2;
    }
    size_t size;
    if (!rv_size_multiply(capacity, vector->element_size, &size)) {
        return false;
    }
    void *data = vector->allocator.resize(vector->allocator.context, vector->data, size);
    if (data == NULL) {
        return false;
    }
    vector->data = data;
    vector->capacity = capacity;
    return true;
}

void *rv_vector_append(RvVector *vector) {
    size_t length;
    if (!rv_size_add(vector->length, 1, &length) || !rv_vector_reserve(vector, length)) {
        return NULL;
    }
    void *element = vector->data + vector->length * vector->element_size;
    vector->length = length;
    return element;
}

void *rv_vector_at(const RvVector *vector, size_t index) {
    if (index >= vector->length) {
        return NULL;
    }
    return vector->data + index * vector->element_size;
}

void rv_vector_destroy(RvVector *vector) {
    if (vector->data != NULL) {
        vector->allocator.release(vector->allocator.context, vector->data);
    }
    vector->data = NULL;
    vector->length = 0;
    vector->capacity = 0;
}
