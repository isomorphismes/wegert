#ifndef WEGERT_OVERLAY_RENDERER_GLES_H
#define WEGERT_OVERLAY_RENDERER_GLES_H

#include <stdbool.h>
#include <stddef.h>

#include <GLES3/gl3.h>

#include "wegert_scene.h"

struct wegert_overlay_renderer_gles {
    GLuint program;
    GLuint overlay_texture;
    GLuint clear_button_texture;

    GLint resolution_location;
    GLint origin_location;
    GLint size_location;
    GLint sampler_location;

    int overlay_width;
    int overlay_height;
    int clear_button_width;
    int clear_button_height;

    bool dirty;
    bool unavailable;
};

void wegert_overlay_renderer_gles_mark_dirty(
    struct wegert_overlay_renderer_gles *renderer
);

bool wegert_overlay_renderer_gles_draw(
    struct wegert_overlay_renderer_gles *renderer,
    const struct wegert_scene *scene,
    int density_dpi,
    GLuint fullscreen_vao,
    char *error,
    size_t error_capacity
);

void wegert_overlay_renderer_gles_destroy(
    struct wegert_overlay_renderer_gles *renderer
);

bool wegert_overlay_renderer_gles_formula_contains(
    const struct wegert_overlay_renderer_gles *renderer,
    float x,
    float y
);

bool wegert_overlay_renderer_gles_clear_button_contains(
    const struct wegert_overlay_renderer_gles *renderer,
    const struct wegert_scene *scene,
    int density_dpi,
    float x,
    float y
);

bool wegert_overlay_renderer_gles_clear_button_center(
    const struct wegert_overlay_renderer_gles *renderer,
    const struct wegert_scene *scene,
    int density_dpi,
    float *x,
    float *y
);

#endif
