#include "../test.h"

#include <math.h>
#include <string.h>

bool test_runtime_arrays(void) {
    RvRuntime runtime;
    rv_runtime_init(&runtime, stdout, stderr);
    RvLocation where = {"arrays.ruv", 1, 1};
    TestAllocator state = {0};
    RvAllocator allocator = test_allocator(&state);
    runtime.allocator = (RvRuntimeAllocator){
        allocator.context, allocator.allocate, allocator.resize, allocator.release
    };
    RvArray integers = {0};
    for (int32_t index = 0; index < 10000; ++index) {
        rv_array_push(&runtime, &integers, (RvElement){.integer = index}, where);
    }
    CHECK(state.calls < 20 && runtime.live_allocations == 1);
    CHECK(rv_array_length(&runtime, &integers, where) == 10000);
    for (int32_t index = 0; index < 10000; ++index) {
        CHECK(rv_array_get(&runtime, &integers, index, where).integer == index);
    }
    for (int32_t index = 9999; index >= 0; --index) {
        CHECK(rv_array_pop(&runtime, &integers, where).integer == index);
    }
    CHECK(integers.length == 0 && integers.capacity >= 10000);
    rv_array_release(&runtime, &integers);
    CHECK(runtime.live_allocations == 0 && state.live == 0);

    RvArray texts = {.owns_text = true};
    RvText *original = rv_text_create(&runtime, "original", 8, where);
    rv_array_push(&runtime, &texts, (RvElement){.text = original}, where);
    RvText *copy = rv_array_get(&runtime, &texts, 0, where).text;
    CHECK(copy == original && runtime.live_allocations == 2);
    rv_array_set(&runtime, &texts, 0, (RvElement){.text = copy}, where);
    RvText *popped = rv_array_pop(&runtime, &texts, where).text;
    rv_array_release(&runtime, &texts);
    CHECK(runtime.live_allocations == 1 && strcmp(rv_text_bytes(popped), "original") == 0);
    rv_text_release(&runtime, popped);

    RvArray reals = {0};
    rv_array_push(&runtime, &reals, (RvElement){.real = -0.0}, where);
    CHECK(signbit(rv_array_pop(&runtime, &reals, where).real));
    rv_array_release(&runtime, &reals);
    RvArray flags = {0};
    rv_array_push(&runtime, &flags, (RvElement){.boolean = true}, where);
    CHECK(rv_array_pop(&runtime, &flags, where).boolean);
    rv_array_release(&runtime, &flags);

    RvText *low = rv_text_from_int(&runtime, INT32_MIN, where);
    RvText *real = rv_text_from_real(&runtime, 0.1, where);
    RvText *flag = rv_text_from_bool(&runtime, false, where);
    CHECK(strcmp(rv_text_bytes(low), "-2147483648") == 0);
    CHECK(strcmp(rv_text_bytes(real), "0.10000000000000001") == 0);
    CHECK(strcmp(rv_text_bytes(flag), "false") == 0);
    rv_text_release(&runtime, low);
    rv_text_release(&runtime, real);
    rv_text_release(&runtime, flag);
    CHECK(runtime.live_allocations == 0 && state.live == 0);
    return true;
}
