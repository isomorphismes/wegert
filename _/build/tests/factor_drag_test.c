#include <assert.h>
#include <math.h>
#include <stddef.h>

#include "../factor_drag.h"

static bool nearly_equal(float left, float right) {
    return fabsf(left - right) < 0.0001f;
}

static void test_density_scaled_touch_radius(void) {
    assert(nearly_equal(factor_touch_radius_pixels(160), 24.0f));
    assert(nearly_equal(factor_touch_radius_pixels(320), 48.0f));
    assert(nearly_equal(factor_touch_radius_pixels(1200), 180.0f));
    assert(nearly_equal(factor_touch_radius_pixels(0), 24.0f));
    assert(nearly_equal(factor_touch_radius_pixels(65534), 24.0f));
    assert(nearly_equal(factor_touch_radius_pixels(65535), 24.0f));
}

static void test_nearest_factor_wins_across_kinds(void) {
    const struct factor_viewport viewport = {
        .width = 1000,
        .height = 500,
        .center_x = 0.0f,
        .center_y = 0.0f,
        .half_height = 2.5f
    };
    const float zeros[][2] = {{0.0f, 0.0f}, {-1.0f, 1.0f}};
    const float poles[][2] = {{0.2f, 0.0f}};

    struct factor_target target = nearest_factor_target(
        &viewport,
        zeros,
        2,
        poles,
        1,
        516.0f,
        250.0f,
        24.0f
    );

    assert(target.found);
    assert(target.kind == FACTOR_POLE);
    assert(target.index == 0);
}

static void test_target_outside_radius_is_rejected(void) {
    const struct factor_viewport viewport = {
        .width = 1000,
        .height = 500,
        .center_x = 0.0f,
        .center_y = 0.0f,
        .half_height = 2.5f
    };
    const float zeros[][2] = {{0.0f, 0.0f}};

    struct factor_target target = nearest_factor_target(
        &viewport,
        zeros,
        1,
        NULL,
        0,
        525.0f,
        250.0f,
        24.0f
    );

    assert(!target.found);
    assert(target.index == -1);
}

static void test_target_on_radius_is_included(void) {
    const struct factor_viewport viewport = {
        .width = 1000,
        .height = 500,
        .center_x = 0.0f,
        .center_y = 0.0f,
        .half_height = 2.5f
    };
    const float poles[][2] = {{0.0f, 0.0f}};

    struct factor_target target = nearest_factor_target(
        &viewport,
        NULL,
        0,
        poles,
        1,
        524.0f,
        250.0f,
        24.0f
    );

    assert(target.found);
    assert(target.kind == FACTOR_POLE);
    assert(target.index == 0);
}

static void test_screen_projection_uses_view_center(void) {
    const struct factor_viewport viewport = {
        .width = 1000,
        .height = 500,
        .center_x = 3.0f,
        .center_y = -2.0f,
        .half_height = 2.5f
    };
    const float position[] = {4.0f, -1.0f};
    float screen_x = 0.0f;
    float screen_y = 0.0f;

    assert(factor_screen_position(&viewport, position, &screen_x, &screen_y));
    assert(nearly_equal(screen_x, 600.0f));
    assert(nearly_equal(screen_y, 150.0f));
}

static void test_drag_uses_original_position_and_full_screen_delta(void) {
    const float original_position[] = {1.25f, -0.5f};
    float output[2] = {0.0f, 0.0f};

    dragged_factor_position(original_position, 20.0f, -30.0f, 0.01f, output);

    assert(nearly_equal(output[0], 1.45f));
    assert(nearly_equal(output[1], -0.2f));
}

static void test_drag_threshold_is_strict(void) {
    assert(!drag_threshold_exceeded(12.0f, 0.0f));
    assert(drag_threshold_exceeded(12.0f, 0.01f));
}

int main(void) {
    test_density_scaled_touch_radius();
    test_nearest_factor_wins_across_kinds();
    test_target_outside_radius_is_rejected();
    test_target_on_radius_is_included();
    test_screen_projection_uses_view_center();
    test_drag_uses_original_position_and_full_screen_delta();
    test_drag_threshold_is_strict();
    return 0;
}
