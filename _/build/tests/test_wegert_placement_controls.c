#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "../wegert_placement_controls.h"

static int nearly_equal(float left, float right) {
    return fabsf(left - right) < 0.001f;
}

int main(void) {
    struct wegert_placement_controls controls;
    assert(wegert_placement_controls_layout(720, 1280, &controls));

    assert(nearly_equal(controls.radius, 46.8f));
    assert(nearly_equal(controls.zero_center[0], 64.8f));
    assert(nearly_equal(controls.zero_center[1], 1215.2f));
    assert(nearly_equal(controls.pole_center[0], 168.3f));
    assert(nearly_equal(controls.pole_center[1], 1215.2f));

    enum factor_kind kind = FACTOR_POLE;
    assert(wegert_placement_controls_hit(
        &controls,
        controls.zero_center[0],
        controls.zero_center[1],
        &kind
    ));
    assert(kind == FACTOR_ZERO);

    assert(wegert_placement_controls_hit(
        &controls,
        controls.pole_center[0],
        controls.pole_center[1],
        &kind
    ));
    assert(kind == FACTOR_POLE);

    assert(!wegert_placement_controls_hit(&controls, 360.0f, 640.0f, &kind));
    assert(!wegert_placement_controls_layout(0, 1280, &controls));

    puts("placement control tests passed");
    return 0;
}
