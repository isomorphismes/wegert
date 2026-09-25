#include "wegert_scene.h"

void wegert_scene_initialize_default(struct wegert_scene *scene) {
    wegert_function_initialize_default(&scene->function);
    wegert_view_initialize_default(&scene->view);
    scene->width = 0;
    scene->height = 0;
}

void wegert_scene_resize(struct wegert_scene *scene, int width, int height) {
    scene->width = width;
    scene->height = height;
}

float wegert_scene_aspect(const struct wegert_scene *scene) {
    return wegert_view_aspect(scene->width, scene->height);
}

float wegert_scene_world_units_per_pixel(const struct wegert_scene *scene) {
    return wegert_view_world_units_per_pixel(&scene->view, scene->height);
}

bool wegert_scene_screen_to_complex(
    const struct wegert_scene *scene,
    float screen_x,
    float screen_y,
    float output[2]
) {
    return wegert_view_screen_to_complex(
        &scene->view,
        scene->width,
        scene->height,
        screen_x,
        screen_y,
        output
    );
}

bool wegert_scene_pan_by_pixels(
    struct wegert_scene *scene,
    float delta_x,
    float delta_y
) {
    return wegert_view_pan_by_pixels(
        &scene->view,
        scene->width,
        scene->height,
        delta_x,
        delta_y
    );
}
