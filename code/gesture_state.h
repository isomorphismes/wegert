#ifndef WEGERT_GESTURE_STATE_H
#define WEGERT_GESTURE_STATE_H

#include <stdbool.h>

enum gesture_kind {
    GESTURE_NONE,
    GESTURE_SINGLE,
    GESTURE_FACTOR,
    GESTURE_PINCH,
    GESTURE_CLEAR_BUTTON,
    GESTURE_VIEW_BUTTON,
    GESTURE_BLOCKED
};

static bool gesture_is_ui_hold(enum gesture_kind gesture) {
    return gesture == GESTURE_CLEAR_BUTTON ||
        gesture == GESTURE_VIEW_BUTTON ||
        gesture == GESTURE_BLOCKED;
}

static bool gesture_pointer_down_starts_pinch(enum gesture_kind gesture, int pointer_count) {
    return pointer_count == 2 &&
        (gesture == GESTURE_SINGLE || gesture == GESTURE_FACTOR);
}

static bool gesture_pointer_down_resets(enum gesture_kind gesture, int pointer_count) {
    return pointer_count >= 3 &&
        (gesture == GESTURE_SINGLE || gesture == GESTURE_FACTOR || gesture == GESTURE_PINCH);
}

static bool gesture_view_release_toggles(enum gesture_kind gesture, bool released_inside) {
    return gesture == GESTURE_VIEW_BUTTON && released_inside;
}

static bool gesture_touch_can_capture_factor(bool continuation_view) {
    return !continuation_view;
}

static bool gesture_apply_pinch_zoom(
    float previous_distance,
    float distance,
    float *half_height
) {
    if (previous_distance <= 1.0f || distance <= 1.0f) {
        return false;
    }

    *half_height *= previous_distance / distance;
    if (*half_height < 0.01f) *half_height = 0.01f;
    if (*half_height > 100000.0f) *half_height = 100000.0f;
    return true;
}

#endif
