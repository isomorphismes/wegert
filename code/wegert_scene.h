#ifndef WEGERT_SCENE_H
#define WEGERT_SCENE_H

#include <stdbool.h>

#include "wegert_function.h"
#include "wegert_view.h"

struct wegert_scene {
    struct wegert_function function;
    struct wegert_view view;
    int width;
    int height;
};

void wegert_scene_initialize_default(struct wegert_scene *scene);
void wegert_scene_resize(struct wegert_scene *scene, int width, int height);
float wegert_scene_aspect(const struct wegert_scene *scene);
float wegert_scene_world_units_per_pixel(const struct wegert_scene *scene);

bool wegert_scene_screen_to_complex(
    const struct wegert_scene *scene,
    float screen_x,
    float screen_y,
    float output[2]
);

bool wegert_scene_pan_by_pixels(
    struct wegert_scene *scene,
    float delta_x,
    float delta_y
);

#endif
