#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_FACTORS 64
#include "../factor_snap.h"
#include "../factor_state.h"

int main(void) {
    struct complex_value factors[MAX_FACTORS] = {
        {10.0f, -3.0f},
        {1.0f, 2.0f}
    };

    struct complex_value near_second = {1.3f, 2.4f};
    assert(factor_snap_to_nearest(&near_second, factors, 2, 0.25f, 4.0f) == 1);
    assert(near_second.real == factors[1].real);
    assert(near_second.imaginary == factors[1].imaginary);

    struct complex_value outside = {2.1f, 2.0f};
    assert(factor_snap_to_nearest(&outside, factors, 2, 0.25f, 4.0f) < 0);
    assert(outside.real == 2.1f && outside.imaginary == 2.0f);

    struct complex_value no_scale = {1.0f, 2.0f};
    assert(factor_snap_to_nearest(&no_scale, factors, 2, 0.0f, 4.0f) < 0);

    struct complex_value inserted_zeros[MAX_FACTORS] = {{0.0f, 0.0f}};
    struct complex_value inserted_poles[MAX_FACTORS] = {{3.0f, 4.0f}};
    int inserted_zero_count = 0;
    int inserted_pole_count = 1;
    struct complex_value near_opposite = {3.000001f, 4.0f};
    assert(factor_snap_to_nearest(
        &near_opposite,
        inserted_poles,
        inserted_pole_count,
        1.0e-6f,
        2.0f
    ) == 0);
    assert(factor_insert_reduced(
        inserted_zeros,
        &inserted_zero_count,
        inserted_poles,
        &inserted_pole_count,
        near_opposite.real,
        near_opposite.imaginary
    ) == FACTOR_CANCELLED_OPPOSITE);
    assert(inserted_zero_count == 0 && inserted_pole_count == 0);

    puts("factor snap tests passed");
    return 0;
}
