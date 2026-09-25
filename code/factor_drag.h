#ifndef WEGERT_FACTOR_DRAG_H
#define WEGERT_FACTOR_DRAG_H

#include <stdbool.h>

#include "wegert_view.h"

#define FACTOR_TOUCH_RADIUS_DP 24.0f
#define DRAG_THRESHOLD_PIXELS 12.0f

enum factor_kind {
    FACTOR_ZERO,
    FACTOR_POLE
};

struct factor_target {
    bool found;
    enum factor_kind kind;
    int index;
};

static inline float factor_touch_radius_pixels(int density_dpi) {
    float density_scale = 1.0f;
    /* AConfiguration reserves 0xfffe and 0xffff for "any" and "none". */
    bool has_concrete_density =
        density_dpi > 0 && density_dpi != 0xfffe && density_dpi != 0xffff;
    if (has_concrete_density) {
        density_scale = (float)density_dpi / 160.0f;
    }
    return FACTOR_TOUCH_RADIUS_DP * density_scale;
}

static inline bool factor_screen_position(
    const struct wegert_view *view,
    int width,
    int height,
    const float position[2],
    float *screen_x,
    float *screen_y
) {
    if (width <= 0 || height <= 0 || view->half_height <= 0.0f) {
        return false;
    }

    float pixels_per_world_unit =
        (float)height / (2.0f * view->half_height);
    *screen_x = 0.5f * (float)width
        + (position[0] - view->center[0]) * pixels_per_world_unit;
    *screen_y = 0.5f * (float)height
        - (position[1] - view->center[1]) * pixels_per_world_unit;
    return true;
}

static inline struct factor_target nearest_factor_target(
    const struct wegert_view *view,
    int width,
    int height,
    const float (*zeros)[2],
    int zero_count,
    const float (*poles)[2],
    int pole_count,
    float touch_x,
    float touch_y,
    float radius_pixels
) {
    struct factor_target target = {
        .found = false,
        .kind = FACTOR_ZERO,
        .index = -1
    };
    if (radius_pixels < 0.0f) {
        return target;
    }

    float radius_squared = radius_pixels * radius_pixels;
    float nearest_distance_squared = radius_squared;
    for (int index = 0; index < zero_count; ++index) {
        float screen_x = 0.0f;
        float screen_y = 0.0f;
        if (!factor_screen_position(
            view, width, height, zeros[index], &screen_x, &screen_y
        )) {
            return target;
        }
        float delta_x = screen_x - touch_x;
        float delta_y = screen_y - touch_y;
        float distance_squared = delta_x * delta_x + delta_y * delta_y;
        if (
            distance_squared <= radius_squared &&
            (!target.found || distance_squared < nearest_distance_squared)
        ) {
            target.found = true;
            target.kind = FACTOR_ZERO;
            target.index = index;
            nearest_distance_squared = distance_squared;
        }
    }

    for (int index = 0; index < pole_count; ++index) {
        float screen_x = 0.0f;
        float screen_y = 0.0f;
        if (!factor_screen_position(
            view, width, height, poles[index], &screen_x, &screen_y
        )) {
            return target;
        }
        float delta_x = screen_x - touch_x;
        float delta_y = screen_y - touch_y;
        float distance_squared = delta_x * delta_x + delta_y * delta_y;
        if (
            distance_squared <= radius_squared &&
            (!target.found || distance_squared < nearest_distance_squared)
        ) {
            target.found = true;
            target.kind = FACTOR_POLE;
            target.index = index;
            nearest_distance_squared = distance_squared;
        }
    }
    return target;
}

static inline bool drag_threshold_exceeded(float delta_x, float delta_y) {
    return delta_x * delta_x + delta_y * delta_y
        > DRAG_THRESHOLD_PIXELS * DRAG_THRESHOLD_PIXELS;
}

static inline void dragged_factor_position(
    const float original_position[2],
    float screen_delta_x,
    float screen_delta_y,
    float world_units_per_pixel,
    float output[2]
) {
    output[0] = original_position[0] + screen_delta_x * world_units_per_pixel;
    output[1] = original_position[1] - screen_delta_y * world_units_per_pixel;
}

#endif
