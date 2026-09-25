#ifndef WEGERT_PORTRAIT_RENDERER_GLES_H
#define WEGERT_PORTRAIT_RENDERER_GLES_H

#include <stdbool.h>
#include <stddef.h>

#include <GLES3/gl3.h>

#include "wegert_scene.h"

struct wegert_portrait_renderer_gles {
    GLuint program;
    GLuint vao;
    GLuint vbo;

    GLint center_location;
    GLint half_height_location;
    GLint aspect_location;
    GLint resolution_location;
    GLint zero_count_location;
    GLint pole_count_location;
    GLint zeros_location;
    GLint poles_location;
};

bool wegert_portrait_renderer_gles_initialize(
    struct wegert_portrait_renderer_gles *renderer,
    const char *fragment_shader_source,
    char *error,
    size_t error_capacity
);

void wegert_portrait_renderer_gles_destroy(
    struct wegert_portrait_renderer_gles *renderer
);

/*
 * Render one mathematical Wegert portrait into the currently bound GLES
 * framebuffer.  This operation knows nothing about Android, gestures, UI
 * controls, overlays, screenshots, icons, or movie encoding.
 */
bool wegert_portrait_renderer_gles_render_frame(
    const struct wegert_portrait_renderer_gles *renderer,
    const struct wegert_scene *scene
);

#endif
