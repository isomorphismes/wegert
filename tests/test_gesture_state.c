#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "../app/src/main/cpp/gesture_state.h"

static bool nearly_equal(float left, float right) {
    return fabsf(left - right) < 0.0001f;
}

int main(void) {
    enum gesture_kind ui_holds[] = {
        GESTURE_BLOCKED,
        GESTURE_CLEAR_BUTTON,
        GESTURE_VIEW_BUTTON
    };
    for (int index = 0; index < 3; ++index) {
        enum gesture_kind gesture = ui_holds[index];
        assert(gesture_is_ui_hold(gesture));
        assert(!gesture_pointer_down_starts_pinch(gesture, 2));
        assert(!gesture_pointer_down_resets(gesture, 3));
    }

    assert(gesture_pointer_down_starts_pinch(GESTURE_SINGLE, 2));
    assert(gesture_pointer_down_starts_pinch(GESTURE_FACTOR, 2));
    assert(!gesture_pointer_down_starts_pinch(GESTURE_SINGLE, 3));
    assert(gesture_pointer_down_resets(GESTURE_SINGLE, 3));
    assert(gesture_pointer_down_resets(GESTURE_FACTOR, 3));
    assert(gesture_pointer_down_resets(GESTURE_PINCH, 3));
    assert(!gesture_pointer_down_resets(GESTURE_NONE, 3));

    assert(!gesture_view_release_toggles(GESTURE_VIEW_BUTTON, false));
    assert(gesture_view_release_toggles(GESTURE_VIEW_BUTTON, true));
    assert(!gesture_view_release_toggles(GESTURE_BLOCKED, true));

    assert(gesture_touch_can_capture_factor(false));
    assert(!gesture_touch_can_capture_factor(true));

    float half_height = 3.5f;
    assert(gesture_apply_pinch_zoom(100.0f, 200.0f, &half_height));
    assert(nearly_equal(half_height, 1.75f));

    assert(gesture_apply_pinch_zoom(200.0f, 100.0f, &half_height));
    assert(nearly_equal(half_height, 3.5f));

    half_height = 3.5f;
    assert(!gesture_apply_pinch_zoom(1.0f, 200.0f, &half_height));
    assert(nearly_equal(half_height, 3.5f));
    assert(!gesture_apply_pinch_zoom(200.0f, 1.0f, &half_height));
    assert(nearly_equal(half_height, 3.5f));

    half_height = 0.02f;
    assert(gesture_apply_pinch_zoom(2.0f, 200.0f, &half_height));
    assert(nearly_equal(half_height, 0.01f));

    half_height = 90000.0f;
    assert(gesture_apply_pinch_zoom(200.0f, 2.0f, &half_height));
    assert(nearly_equal(half_height, 100000.0f));

    puts("gesture state tests passed");
    return 0;
}
