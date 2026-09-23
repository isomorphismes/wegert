#include "wegert_offscreen_gles.h"

#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_error(
    char *error,
    size_t error_capacity,
    const char *message
) {
    if (error == NULL || error_capacity == 0u) {
        return;
    }
    snprintf(error, error_capacity, "%s", message);
}

static void set_egl_error(
    char *error,
    size_t error_capacity,
    const char *message
) {
    if (error == NULL || error_capacity == 0u) {
        return;
    }
    snprintf(
        error,
        error_capacity,
        "%s: EGL error 0x%04x",
        message,
        (unsigned int)eglGetError()
    );
}

static EGLDisplay initialize_display(char *error, size_t error_capacity) {
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display != EGL_NO_DISPLAY && eglInitialize(display, NULL, NULL)) {
        return display;
    }

#ifdef EGL_PLATFORM_SURFACELESS_MESA
    display = eglGetPlatformDisplay(
        EGL_PLATFORM_SURFACELESS_MESA,
        EGL_DEFAULT_DISPLAY,
        NULL
    );
    if (display != EGL_NO_DISPLAY && eglInitialize(display, NULL, NULL)) {
        return display;
    }
#endif

    set_egl_error(error, error_capacity, "could not initialize EGL display");
    return EGL_NO_DISPLAY;
}

bool wegert_offscreen_gles_initialize(
    struct wegert_offscreen_gles *offscreen,
    int width,
    int height,
    const char *fragment_shader_source,
    char *error,
    size_t error_capacity
) {
    memset(offscreen, 0, sizeof(*offscreen));
    offscreen->display = EGL_NO_DISPLAY;
    offscreen->surface = EGL_NO_SURFACE;
    offscreen->context = EGL_NO_CONTEXT;

    if (width <= 0 || height <= 0) {
        set_error(error, error_capacity, "offscreen dimensions must be positive");
        return false;
    }

    EGLDisplay display = initialize_display(error, error_capacity);
    if (display == EGL_NO_DISPLAY) {
        return false;
    }

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        set_egl_error(error, error_capacity, "could not bind OpenGL ES API");
        eglTerminate(display);
        return false;
    }

    const EGLint config_attributes[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    EGLConfig config = NULL;
    EGLint config_count = 0;
    if (
        !eglChooseConfig(
            display,
            config_attributes,
            &config,
            1,
            &config_count
        ) ||
        config_count < 1
    ) {
        set_egl_error(error, error_capacity, "could not choose pbuffer config");
        eglTerminate(display);
        return false;
    }

    const EGLint pbuffer_attributes[] = {
        EGL_WIDTH, width,
        EGL_HEIGHT, height,
        EGL_NONE
    };
    EGLSurface surface = eglCreatePbufferSurface(
        display,
        config,
        pbuffer_attributes
    );
    if (surface == EGL_NO_SURFACE) {
        set_egl_error(error, error_capacity, "could not create pbuffer surface");
        eglTerminate(display);
        return false;
    }

    const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    EGLContext context = eglCreateContext(
        display,
        config,
        EGL_NO_CONTEXT,
        context_attributes
    );
    if (context == EGL_NO_CONTEXT) {
        set_egl_error(error, error_capacity, "could not create GLES3 context");
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return false;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        set_egl_error(error, error_capacity, "could not make pbuffer context current");
        eglDestroyContext(display, context);
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return false;
    }

    offscreen->display = display;
    offscreen->surface = surface;
    offscreen->context = context;
    offscreen->width = width;
    offscreen->height = height;

    if (!wegert_portrait_renderer_gles_initialize(
        &offscreen->portrait_renderer,
        fragment_shader_source,
        error,
        error_capacity
    )) {
        wegert_offscreen_gles_destroy(offscreen);
        return false;
    }

    return true;
}

bool wegert_offscreen_gles_render_rgba(
    struct wegert_offscreen_gles *offscreen,
    const struct wegert_scene *scene,
    uint8_t *rgba,
    size_t rgba_capacity,
    char *error,
    size_t error_capacity
) {
    if (
        offscreen->display == EGL_NO_DISPLAY ||
        offscreen->surface == EGL_NO_SURFACE ||
        offscreen->context == EGL_NO_CONTEXT
    ) {
        set_error(error, error_capacity, "offscreen renderer is not initialized");
        return false;
    }
    if (
        scene->width != offscreen->width ||
        scene->height != offscreen->height
    ) {
        set_error(
            error,
            error_capacity,
            "scene dimensions do not match the offscreen surface"
        );
        return false;
    }

    size_t row_bytes = (size_t)scene->width * 4u;
    size_t required = row_bytes * (size_t)scene->height;
    if (rgba == NULL || rgba_capacity < required) {
        set_error(error, error_capacity, "RGBA output buffer is too small");
        return false;
    }

    if (!eglMakeCurrent(
        offscreen->display,
        offscreen->surface,
        offscreen->surface,
        offscreen->context
    )) {
        set_egl_error(error, error_capacity, "could not make pbuffer current");
        return false;
    }

    if (!wegert_portrait_renderer_gles_render_frame(
        &offscreen->portrait_renderer,
        scene
    )) {
        set_error(error, error_capacity, "portrait frame render failed");
        return false;
    }

    glFinish();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(
        0,
        0,
        scene->width,
        scene->height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        rgba
    );
    GLenum gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        if (error != NULL && error_capacity > 0u) {
            snprintf(
                error,
                error_capacity,
                "glReadPixels failed: GL error 0x%04x",
                (unsigned int)gl_error
            );
        }
        return false;
    }

    uint8_t *row = malloc(row_bytes);
    if (row == NULL) {
        set_error(error, error_capacity, "could not allocate row-flip buffer");
        return false;
    }

    for (int y = 0; y < scene->height / 2; ++y) {
        uint8_t *top = rgba + (size_t)y * row_bytes;
        uint8_t *bottom =
            rgba + (size_t)(scene->height - 1 - y) * row_bytes;
        memcpy(row, top, row_bytes);
        memcpy(top, bottom, row_bytes);
        memcpy(bottom, row, row_bytes);
    }
    free(row);

    return true;
}

void wegert_offscreen_gles_destroy(struct wegert_offscreen_gles *offscreen) {
    if (
        offscreen->display != EGL_NO_DISPLAY &&
        offscreen->context != EGL_NO_CONTEXT &&
        offscreen->surface != EGL_NO_SURFACE
    ) {
        (void)eglMakeCurrent(
            offscreen->display,
            offscreen->surface,
            offscreen->surface,
            offscreen->context
        );
        wegert_portrait_renderer_gles_destroy(
            &offscreen->portrait_renderer
        );
    }

    if (
        offscreen->display != EGL_NO_DISPLAY &&
        offscreen->context != EGL_NO_CONTEXT
    ) {
        eglDestroyContext(offscreen->display, offscreen->context);
    }
    if (
        offscreen->display != EGL_NO_DISPLAY &&
        offscreen->surface != EGL_NO_SURFACE
    ) {
        eglDestroySurface(offscreen->display, offscreen->surface);
    }
    if (offscreen->display != EGL_NO_DISPLAY) {
        eglTerminate(offscreen->display);
    }

    offscreen->display = EGL_NO_DISPLAY;
    offscreen->surface = EGL_NO_SURFACE;
    offscreen->context = EGL_NO_CONTEXT;
    offscreen->width = 0;
    offscreen->height = 0;
}
