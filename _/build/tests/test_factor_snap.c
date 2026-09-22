#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_FACTORS 64
#include "../factor_snap.h"
#include "../factor_state.h"

int main(void) {
    float factors[MAX_FACTORS][2] = {
        {10.0f, -3.0f},
        {1.0f, 2.0f}
    };

    float near_second[2] = {1.3f, 2.4f};
    assert(factor_snap_to_nearest(near_second, factors, 2, 0.25f, 4.0f) == 1);
    assert(near_second[0] == factors[1][0]);
    assert(near_second[1] == factors[1][1]);

    float outside[2] = {2.1f, 2.0f};
    assert(factor_snap_to_nearest(outside, factors, 2, 0.25f, 4.0f) < 0);
    assert(outside[0] == 2.1f && outside[1] == 2.0f);

    float no_scale[2] = {1.0f, 2.0f};
    assert(factor_snap_to_nearest(no_scale, factors, 2, 0.0f, 4.0f) < 0);

    float inserted_zeros[MAX_FACTORS][2] = {{0.0f, 0.0f}};
    float inserted_poles[MAX_FACTORS][2] = {{3.0f, 4.0f}};
    int inserted_zero_count = 0;
    int inserted_pole_count = 1;
    float near_opposite[2] = {3.000001f, 4.0f};
    assert(factor_snap_to_nearest(
        near_opposite,
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
        near_opposite[0],
        near_opposite[1]
    ) == FACTOR_CANCELLED_OPPOSITE);
    assert(inserted_zero_count == 0 && inserted_pole_count == 0);

    puts("factor snap tests passed");
    return 0;
}
