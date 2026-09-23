#ifndef WEGERT_VIEW_H
#define WEGERT_VIEW_H

#include <stdbool.h>

struct wegert_view {
    float center[2];
    float half_height;
};

void wegert_view_initialize_default(struct wegert_view *view);

float wegert_view_aspect(int width, int height);

float wegert_view_world_units_per_pixel(
    const struct wegert_view *view,
    int height
);

bool wegert_view_screen_to_complex(
    const struct wegert_view *view,
    int width,
    int height,
    float screen_x,
    float screen_y,
    float output[2]
);

bool wegert_view_pan_by_pixels(
    struct wegert_view *view,
    int width,
    int height,
    float delta_x,
    float delta_y
);

#endif
