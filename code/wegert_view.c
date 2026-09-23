#include "wegert_view.h"

void wegert_view_initialize_default(struct wegert_view *view) {
    view->center[0] = 0.0f;
    view->center[1] = 0.0f;
    view->half_height = 3.5f;
}

float wegert_view_aspect(int width, int height) {
    if (width <= 0 || height <= 0) {
        return 0.0f;
    }
    return (float)width / (float)height;
}

float wegert_view_world_units_per_pixel(
    const struct wegert_view *view,
    int height
) {
    if (height <= 0 || view->half_height <= 0.0f) {
        return 0.0f;
    }
    return 2.0f * view->half_height / (float)height;
}

bool wegert_view_screen_to_complex(
    const struct wegert_view *view,
    int width,
    int height,
    float screen_x,
    float screen_y,
    float output[2]
) {
    if (width <= 0 || height <= 0 || view->half_height <= 0.0f) {
        return false;
    }

    float aspect = wegert_view_aspect(width, height);
    float ndc_x = 2.0f * screen_x / (float)width - 1.0f;
    float ndc_y = 1.0f - 2.0f * screen_y / (float)height;

    output[0] = view->center[0] + ndc_x * view->half_height * aspect;
    output[1] = view->center[1] + ndc_y * view->half_height;
    return true;
}

bool wegert_view_pan_by_pixels(
    struct wegert_view *view,
    int width,
    int height,
    float delta_x,
    float delta_y
) {
    if (width <= 0 || height <= 0 || view->half_height <= 0.0f) {
        return false;
    }

    float aspect = wegert_view_aspect(width, height);
    view->center[0] -=
        2.0f * delta_x * view->half_height * aspect / (float)width;
    view->center[1] +=
        2.0f * delta_y * view->half_height / (float)height;
    return true;
}
