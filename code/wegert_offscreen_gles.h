#ifndef WEGERT_OFFSCREEN_GLES_H
#define WEGERT_OFFSCREEN_GLES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <EGL/egl.h>

#include "wegert_portrait_renderer_gles.h"
#include "wegert_scene.h"

struct wegert_offscreen_gles {
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int width;
    int height;
    struct wegert_portrait_renderer_gles portrait_renderer;
};

bool wegert_offscreen_gles_initialize(
    struct wegert_offscreen_gles *offscreen,
    int width,
    int height,
    const char *fragment_shader_source,
    char *error,
    size_t error_capacity
);

bool wegert_offscreen_gles_render_rgba(
    struct wegert_offscreen_gles *offscreen,
    const struct wegert_scene *scene,
    uint8_t *rgba,
    size_t rgba_capacity,
    char *error,
    size_t error_capacity
);

void wegert_offscreen_gles_destroy(struct wegert_offscreen_gles *offscreen);

#endif
