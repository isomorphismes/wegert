#include <android/asset_manager.h>
#include <android/configuration.h>
#include <android/input.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "wegert_scene.h"
#include "wegert_gles.h"
#include "wegert_portrait_renderer_gles.h"
#include "wegert_placement_controls.h"
#include "factor_drag.h"

#define LOG_TAG "Wegert"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#include "factor_snap.h"
#include "factor_state.h"
#include "gesture_state.h"

static const char *PLACEMENT_CONTROL_FRAGMENT_SHADER =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec2 v_ndc;\n"
    "uniform vec2 u_resolution;\n"
    "uniform vec2 u_zero_center;\n"
    "uniform vec2 u_pole_center;\n"
    "uniform float u_radius;\n"
    "uniform int u_placement_kind;\n"
    "out vec4 out_color;\n"
    "float circle_mask(vec2 point, vec2 center, float radius) {\n"
    "    return 1.0 - smoothstep(radius - 1.5, radius + 1.5, length(point - center));\n"
    "}\n"
    "float line_mask(vec2 point, vec2 start, vec2 finish, float half_width) {\n"
    "    vec2 segment = finish - start;\n"
    "    float along = clamp(dot(point - start, segment) / dot(segment, segment), 0.0, 1.0);\n"
    "    float distance_to_line = length(point - (start + along * segment));\n"
    "    return 1.0 - smoothstep(half_width - 1.0, half_width + 1.0, distance_to_line);\n"
    "}\n"
    "vec4 button(vec2 point, vec2 center, bool selected, bool pole) {\n"
    "    float disk = circle_mask(point, center, u_radius);\n"
    "    float rim = circle_mask(point, center, u_radius) - circle_mask(point, center, u_radius - 3.0);\n"
    "    vec4 background = selected ? vec4(0.96, 0.96, 0.93, 0.94) : vec4(0.05, 0.05, 0.05, 0.72);\n"
    "    vec3 mark_color = selected ? vec3(0.05) : vec3(0.96);\n"
    "    float mark = 0.0;\n"
    "    if (pole) {\n"
    "        float reach = u_radius * 0.38;\n"
    "        float width = max(2.5, u_radius * 0.075);\n"
    "        mark = max(\n"
    "            line_mask(point, center - vec2(reach), center + vec2(reach), width),\n"
    "            line_mask(point, center + vec2(-reach, reach), center + vec2(reach, -reach), width)\n"
    "        );\n"
    "    } else {\n"
    "        float outer = circle_mask(point, center, u_radius * 0.40);\n"
    "        float inner = circle_mask(point, center, u_radius * 0.27);\n"
    "        mark = outer - inner;\n"
    "    }\n"
    "    vec4 color = background * disk;\n"
    "    color.rgb = mix(color.rgb, vec3(0.96), rim);\n"
    "    color.rgb = mix(color.rgb, mark_color, mark);\n"
    "    color.a = max(color.a, max(rim, mark));\n"
    "    return color;\n"
    "}\n"
    "void main() {\n"
    "    vec2 point = vec2(gl_FragCoord.x, u_resolution.y - gl_FragCoord.y);\n"
    "    vec4 zero_button = button(point, u_zero_center, u_placement_kind == 0, false);\n"
    "    vec4 pole_button = button(point, u_pole_center, u_placement_kind == 1, true);\n"
    "    out_color = zero_button.a >= pole_button.a ? zero_button : pole_button;\n"
    "}\n";

struct engine {
    struct android_app *app;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    struct wegert_portrait_renderer_gles portrait_renderer;

    GLuint vao;
    GLuint vbo;
    GLuint placement_program;
    GLint placement_resolution_location;
    GLint placement_zero_center_location;
    GLint placement_pole_center_location;
    GLint placement_radius_location;
    GLint placement_kind_location;

    GLuint overlay_program;
    GLuint overlay_texture;
    GLuint clear_button_texture;
    GLint overlay_resolution_location;
    GLint overlay_origin_location;
    GLint overlay_size_location;
    GLint overlay_sampler_location;
    int overlay_width;
    int overlay_height;
    int clear_button_width;
    int clear_button_height;
    bool overlay_dirty;
    bool overlay_unavailable;

    struct wegert_scene scene;
    enum factor_kind placement_kind;

    enum gesture_kind gesture;
    bool moved;
    float down_x;
    float down_y;
    float last_x;
    float last_y;
    enum factor_kind captured_factor_kind;
    int captured_factor_index;
    float captured_factor_original[2];
    float captured_factor_world_units_per_pixel;
    float pinch_last_distance;
    float pinch_last_mid_x;
    float pinch_last_mid_y;

    bool dirty;
    bool logged_first_frame;
};

static void initialize_scene(struct engine *engine) {
    wegert_scene_initialize_default(&engine->scene);
    engine->placement_kind = FACTOR_ZERO;
    engine->overlay_dirty = true;
    engine->dirty = true;
}

static void reset_all(struct engine *engine) {
    initialize_scene(engine);
    LOGI("default function and camera reset");
}

static void clear_function(struct engine *engine) {
    wegert_function_clear(&engine->scene.function);
    engine->overlay_dirty = true;
    engine->dirty = true;
}

static char *load_asset_text(AAssetManager *manager, const char *name) {
    AAsset *asset = AAssetManager_open(manager, name, AASSET_MODE_BUFFER);
    if (asset == NULL) {
        LOGE("could not open asset %s", name);
        return NULL;
    }

    off64_t length = AAsset_getLength64(asset);
    char *text = malloc((size_t)length + 1u);
    if (text == NULL) {
        AAsset_close(asset);
        return NULL;
    }

    off64_t offset = 0;
    while (offset < length) {
        int amount = AAsset_read(asset, text + offset, (size_t)(length - offset));
        if (amount <= 0) {
            free(text);
            AAsset_close(asset);
            LOGE("could not read asset %s", name);
            return NULL;
        }
        offset += amount;
    }

    text[length] = '\0';
    AAsset_close(asset);
    return text;
}

#include "polynomial_overlay.h"

static bool create_renderer(struct engine *engine) {
    char *fragment_source = load_asset_text(
        engine->app->activity->assetManager,
        "wegert.frag"
    );
    if (fragment_source == NULL) {
        return false;
    }

    char error[2048] = {0};
    bool portrait_ready = wegert_portrait_renderer_gles_initialize(
        &engine->portrait_renderer,
        fragment_source,
        error,
        sizeof(error)
    );
    free(fragment_source);
    if (!portrait_ready) {
        LOGE("%s", error[0] != '\0' ? error : "portrait renderer unavailable");
        return false;
    }

    GLuint placement_vertex_shader = 0;
    GLuint placement_fragment_shader = 0;
    bool placement_vertex_ready = wegert_gles_compile_shader(
        GL_VERTEX_SHADER,
        WEGERT_FULLSCREEN_VERTEX_SHADER,
        &placement_vertex_shader,
        error,
        sizeof(error)
    );
    bool placement_fragment_ready = wegert_gles_compile_shader(
        GL_FRAGMENT_SHADER,
        PLACEMENT_CONTROL_FRAGMENT_SHADER,
        &placement_fragment_shader,
        error,
        sizeof(error)
    );
    if (placement_vertex_ready && placement_fragment_ready) {
        if (!wegert_gles_link_program(
            placement_vertex_shader,
            placement_fragment_shader,
            &engine->placement_program,
            error,
            sizeof(error)
        )) {
            LOGE("%s", error);
        }
    } else {
        LOGE("%s", error);
    }
    if (placement_vertex_shader != 0) glDeleteShader(placement_vertex_shader);
    if (placement_fragment_shader != 0) glDeleteShader(placement_fragment_shader);

    if (engine->placement_program == 0) {
        LOGE("placement controls unavailable");
    } else {
        engine->placement_resolution_location = glGetUniformLocation(
            engine->placement_program,
            "u_resolution"
        );
        engine->placement_zero_center_location = glGetUniformLocation(
            engine->placement_program,
            "u_zero_center"
        );
        engine->placement_pole_center_location = glGetUniformLocation(
            engine->placement_program,
            "u_pole_center"
        );
        engine->placement_radius_location = glGetUniformLocation(
            engine->placement_program,
            "u_radius"
        );
        engine->placement_kind_location = glGetUniformLocation(
            engine->placement_program,
            "u_placement_kind"
        );
    }

    if (!wegert_gles_create_fullscreen_triangle(&engine->vao, &engine->vbo)) {
        LOGE("could not create UI fullscreen triangle");
        wegert_portrait_renderer_gles_destroy(&engine->portrait_renderer);
        return false;
    }

    LOGI(
        "renderer ready: GL_VERSION=%s GL_RENDERER=%s",
        glGetString(GL_VERSION),
        glGetString(GL_RENDERER)
    );
    return true;
}

static bool initialize_display(struct engine *engine) {
    if (engine->app->window == NULL) {
        return false;
    }

    const EGLint config_attributes[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY || !eglInitialize(display, NULL, NULL)) {
        LOGE("eglInitialize failed: 0x%x", eglGetError());
        return false;
    }

    EGLConfig config = NULL;
    EGLint config_count = 0;
    if (!eglChooseConfig(display, config_attributes, &config, 1, &config_count) || config_count != 1) {
        LOGE("could not choose GLES3 EGL config: 0x%x", eglGetError());
        eglTerminate(display);
        return false;
    }

    EGLint format = 0;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

    EGLSurface surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
    if (surface == EGL_NO_SURFACE || context == EGL_NO_CONTEXT) {
        LOGE("could not create EGL surface/context: 0x%x", eglGetError());
        if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
        if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
        eglTerminate(display);
        return false;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOGE("eglMakeCurrent failed: 0x%x", eglGetError());
        eglDestroyContext(display, context);
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return false;
    }

    engine->display = display;
    engine->surface = surface;
    engine->context = context;
    eglQuerySurface(display, surface, EGL_WIDTH, &engine->scene.width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &engine->scene.height);
    LOGI("EGL surface ready: %dx%d", engine->scene.width, engine->scene.height);

    if (!create_renderer(engine)) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(display, context);
        eglDestroySurface(display, surface);
        eglTerminate(display);
        engine->display = EGL_NO_DISPLAY;
        engine->surface = EGL_NO_SURFACE;
        engine->context = EGL_NO_CONTEXT;
        return false;
    }

    glViewport(0, 0, engine->scene.width, engine->scene.height);
    engine->overlay_dirty = true;
    engine->dirty = true;
    return true;
}

static void terminate_display(struct engine *engine) {
    if (engine->display == EGL_NO_DISPLAY) {
        return;
    }

    polynomial_overlay_destroy(engine);
    wegert_portrait_renderer_gles_destroy(&engine->portrait_renderer);
    wegert_gles_destroy_fullscreen_triangle(&engine->vao, &engine->vbo);
    if (engine->placement_program != 0) {
        glDeleteProgram(engine->placement_program);
        engine->placement_program = 0;
    }

    eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (engine->context != EGL_NO_CONTEXT) {
        eglDestroyContext(engine->display, engine->context);
    }
    if (engine->surface != EGL_NO_SURFACE) {
        eglDestroySurface(engine->display, engine->surface);
    }
    eglTerminate(engine->display);

    engine->display = EGL_NO_DISPLAY;
    engine->surface = EGL_NO_SURFACE;
    engine->context = EGL_NO_CONTEXT;
}

static void update_surface_size(struct engine *engine) {
    if (engine->display == EGL_NO_DISPLAY || engine->surface == EGL_NO_SURFACE) {
        return;
    }
    eglQuerySurface(engine->display, engine->surface, EGL_WIDTH, &engine->scene.width);
    eglQuerySurface(engine->display, engine->surface, EGL_HEIGHT, &engine->scene.height);
    glViewport(0, 0, engine->scene.width, engine->scene.height);
    engine->overlay_dirty = true;
    engine->dirty = true;
}

static void draw_frame(struct engine *engine) {
    if (
        engine->display == EGL_NO_DISPLAY ||
        engine->scene.width <= 0 ||
        engine->scene.height <= 0
    ) {
        return;
    }

    if (!wegert_portrait_renderer_gles_render_frame(
        &engine->portrait_renderer,
        &engine->scene
    )) {
        return;
    }

    polynomial_overlay_draw(engine);

    struct wegert_placement_controls placement_controls;
    bool have_placement_controls = wegert_placement_controls_layout(
        engine->scene.width,
        engine->scene.height,
        &placement_controls
    );

    if (engine->placement_program != 0 && have_placement_controls) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(engine->placement_program);
        glUniform2f(
            engine->placement_resolution_location,
            (float)engine->scene.width,
            (float)engine->scene.height
        );
        glUniform2f(
            engine->placement_zero_center_location,
            placement_controls.zero_center[0],
            placement_controls.zero_center[1]
        );
        glUniform2f(
            engine->placement_pole_center_location,
            placement_controls.pole_center[0],
            placement_controls.pole_center[1]
        );
        glUniform1f(
            engine->placement_radius_location,
            placement_controls.radius
        );
        glUniform1i(engine->placement_kind_location, (int)engine->placement_kind);
        glBindVertexArray(engine->vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDisable(GL_BLEND);
    }

    if (!engine->logged_first_frame) {
        if (have_placement_controls) {
            LOGI(
                "placement control centers: zero=%d,%d pole=%d,%d",
                (int)placement_controls.zero_center[0],
                (int)placement_controls.zero_center[1],
                (int)placement_controls.pole_center[0],
                (int)placement_controls.pole_center[1]
            );
        }

        GLubyte pixel[4] = {0, 0, 0, 0};
        glReadPixels(engine->scene.width / 2, engine->scene.height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        GLenum error = glGetError();
        LOGI("first frame: center rgba=%u,%u,%u,%u glError=0x%x",
             pixel[0], pixel[1], pixel[2], pixel[3], error);
        engine->logged_first_frame = true;
    }

    if (!eglSwapBuffers(engine->display, engine->surface)) {
        LOGE("eglSwapBuffers failed: 0x%x", eglGetError());
    }
    engine->dirty = false;
}

static void screen_to_complex(const struct engine *engine, float x, float y, float output[2]) {
    (void)wegert_scene_screen_to_complex(&engine->scene, x, y, output);
}

static float factor_snap_radius_pixels(const struct engine *engine) {
    int density = AConfiguration_getDensity(engine->app->config);
    if (density < 72 || density > 1000) {
        density = 160;
    }

    float radius = 24.0f * (float)density / 160.0f;
    if (radius < 24.0f) radius = 24.0f;
    if (radius > 72.0f) radius = 72.0f;
    return radius;
}

static void snap_touch_to_factors(
    const struct engine *engine,
    float point[2],
    float factors[WEGERT_MAX_FACTORS][2],
    int factor_count
) {
    float world_per_pixel = wegert_scene_world_units_per_pixel(&engine->scene);
    factor_snap_to_nearest(
        point,
        factors,
        factor_count,
        world_per_pixel,
        factor_snap_radius_pixels(engine)
    );
}

static void pan_by_pixels(struct engine *engine, float delta_x, float delta_y) {
    if (wegert_scene_pan_by_pixels(&engine->scene, delta_x, delta_y)) {
        engine->dirty = true;
    }
}

static void add_zero(struct engine *engine, float x, float y) {
    if (engine->scene.width <= 0 || engine->scene.height <= 0) {
        return;
    }

    float factor[2];
    screen_to_complex(engine, x, y, factor);
    snap_touch_to_factors(engine, factor, engine->scene.function.poles, engine->scene.function.pole_count);
    enum factor_change change = factor_insert_reduced(
        engine->scene.function.zeros,
        &engine->scene.function.zero_count,
        engine->scene.function.poles,
        &engine->scene.function.pole_count,
        factor[0],
        factor[1]
    );
    if (change == FACTOR_UNCHANGED) {
        return;
    }

    engine->overlay_dirty = true;
    engine->dirty = true;
}

static void add_pole(struct engine *engine, float x, float y) {
    if (engine->scene.width <= 0 || engine->scene.height <= 0) {
        return;
    }

    float factor[2];
    screen_to_complex(engine, x, y, factor);
    snap_touch_to_factors(engine, factor, engine->scene.function.zeros, engine->scene.function.zero_count);
    enum factor_change change = factor_insert_reduced(
        engine->scene.function.poles,
        &engine->scene.function.pole_count,
        engine->scene.function.zeros,
        &engine->scene.function.zero_count,
        factor[0],
        factor[1]
    );
    if (change == FACTOR_UNCHANGED) {
        return;
    }

    engine->overlay_dirty = true;
    engine->dirty = true;
}

static struct factor_target factor_target_at(
    const struct engine *engine,
    float x,
    float y
) {
    int density_dpi = 0;
    if (engine->app != NULL && engine->app->config != NULL) {
        density_dpi = AConfiguration_getDensity(engine->app->config);
    }
    return nearest_factor_target(
        &engine->scene.view,
        engine->scene.width,
        engine->scene.height,
        engine->scene.function.zeros,
        engine->scene.function.zero_count,
        engine->scene.function.poles,
        engine->scene.function.pole_count,
        x,
        y,
        factor_touch_radius_pixels(density_dpi)
    );
}

static void capture_factor(
    struct engine *engine,
    const struct factor_target *target
) {
    engine->captured_factor_kind = target->kind;
    engine->captured_factor_index = target->index;
    const float *position = target->kind == FACTOR_POLE
        ? engine->scene.function.poles[target->index]
        : engine->scene.function.zeros[target->index];
    engine->captured_factor_original[0] = position[0];
    engine->captured_factor_original[1] = position[1];
    engine->captured_factor_world_units_per_pixel =
        wegert_scene_world_units_per_pixel(&engine->scene);
}

static void move_captured_factor(struct engine *engine, float x, float y) {
    float *position = NULL;
    if (
        engine->captured_factor_kind == FACTOR_ZERO &&
        engine->captured_factor_index >= 0 &&
        engine->captured_factor_index < engine->scene.function.zero_count
    ) {
        position = engine->scene.function.zeros[engine->captured_factor_index];
    } else if (
        engine->captured_factor_kind == FACTOR_POLE &&
        engine->captured_factor_index >= 0 &&
        engine->captured_factor_index < engine->scene.function.pole_count
    ) {
        position = engine->scene.function.poles[engine->captured_factor_index];
    }
    if (position == NULL) {
        return;
    }

    dragged_factor_position(
        engine->captured_factor_original,
        x - engine->down_x,
        y - engine->down_y,
        engine->captured_factor_world_units_per_pixel,
        position
    );
    engine->overlay_dirty = true;
    engine->dirty = true;
}


static float pointer_distance(const AInputEvent *event) {
    float dx = AMotionEvent_getX(event, 0) - AMotionEvent_getX(event, 1);
    float dy = AMotionEvent_getY(event, 0) - AMotionEvent_getY(event, 1);
    return hypotf(dx, dy);
}

static void pointer_midpoint(const AInputEvent *event, float *x, float *y) {
    *x = 0.5f * (AMotionEvent_getX(event, 0) + AMotionEvent_getX(event, 1));
    *y = 0.5f * (AMotionEvent_getY(event, 0) + AMotionEvent_getY(event, 1));
}

static int32_t handle_input(struct android_app *app, AInputEvent *event) {
    struct engine *engine = app->userData;
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }

    int32_t action = AMotionEvent_getAction(event);
    int32_t masked_action = action & AMOTION_EVENT_ACTION_MASK;
    size_t pointer_count = AMotionEvent_getPointerCount(event);

    switch (masked_action) {
        case AMOTION_EVENT_ACTION_DOWN: {
            float x = AMotionEvent_getX(event, 0);
            float y = AMotionEvent_getY(event, 0);
            enum factor_kind selected_kind = FACTOR_ZERO;
            struct wegert_placement_controls placement_controls;
            bool hit_placement_control =
                wegert_placement_controls_layout(
                    engine->scene.width,
                    engine->scene.height,
                    &placement_controls
                ) &&
                wegert_placement_controls_hit(
                    &placement_controls,
                    x,
                    y,
                    &selected_kind
                );
            if (hit_placement_control) {
                engine->placement_kind = selected_kind;
                engine->gesture = GESTURE_BLOCKED;
                engine->moved = false;
                engine->dirty = true;
                return 1;
            }
            if (clear_button_contains(engine, x, y)) {
                engine->gesture = GESTURE_CLEAR_BUTTON;
                engine->moved = false;
                return 1;
            }
            if (polynomial_overlay_contains(engine, x, y)) {
                engine->gesture = GESTURE_BLOCKED;
                engine->moved = false;
                return 1;
            }
            engine->moved = false;
            engine->down_x = x;
            engine->down_y = y;
            engine->last_x = engine->down_x;
            engine->last_y = engine->down_y;
            struct factor_target target = factor_target_at(engine, x, y);
            if (target.found) {
                capture_factor(engine, &target);
                engine->gesture = GESTURE_FACTOR;
            } else {
                engine->gesture = GESTURE_SINGLE;
            }
            return 1;
        }

        case AMOTION_EVENT_ACTION_POINTER_DOWN: {
            if (gesture_is_ui_hold(engine->gesture)) {
                engine->gesture = GESTURE_BLOCKED;
                engine->moved = false;
                return 1;
            }
            if (gesture_pointer_down_resets(engine->gesture, (int)pointer_count)) {
                reset_all(engine);
                engine->gesture = GESTURE_BLOCKED;
                return 1;
            }
            if (gesture_pointer_down_starts_pinch(engine->gesture, (int)pointer_count)) {
                engine->gesture = GESTURE_PINCH;
                engine->moved = false;
                engine->pinch_last_distance = pointer_distance(event);
                pointer_midpoint(event, &engine->pinch_last_mid_x, &engine->pinch_last_mid_y);
                return 1;
            }
            return 0;
        }

        case AMOTION_EVENT_ACTION_MOVE: {
            if (engine->gesture == GESTURE_FACTOR && pointer_count == 1) {
                float x = AMotionEvent_getX(event, 0);
                float y = AMotionEvent_getY(event, 0);
                float from_down_x = x - engine->down_x;
                float from_down_y = y - engine->down_y;
                if (drag_threshold_exceeded(from_down_x, from_down_y)) {
                    engine->moved = true;
                }
                if (engine->moved) {
                    move_captured_factor(engine, x, y);
                }
                return 1;
            }

            if (engine->gesture == GESTURE_SINGLE && pointer_count == 1) {
                float x = AMotionEvent_getX(event, 0);
                float y = AMotionEvent_getY(event, 0);
                float from_down_x = x - engine->down_x;
                float from_down_y = y - engine->down_y;
                if (drag_threshold_exceeded(from_down_x, from_down_y)) {
                    engine->moved = true;
                }
                if (engine->moved) {
                    pan_by_pixels(engine, x - engine->last_x, y - engine->last_y);
                }
                engine->last_x = x;
                engine->last_y = y;
                return 1;
            }

            if (engine->gesture == GESTURE_PINCH && pointer_count >= 2) {
                float midpoint_x = 0.0f;
                float midpoint_y = 0.0f;
                pointer_midpoint(event, &midpoint_x, &midpoint_y);
                float distance = pointer_distance(event);

                pan_by_pixels(
                    engine,
                    midpoint_x - engine->pinch_last_mid_x,
                    midpoint_y - engine->pinch_last_mid_y
                );

                if (gesture_apply_pinch_zoom(
                    engine->pinch_last_distance,
                    distance,
                    &engine->scene.view.half_height
                )) {
                    engine->dirty = true;
                }

                engine->pinch_last_distance = distance;
                engine->pinch_last_mid_x = midpoint_x;
                engine->pinch_last_mid_y = midpoint_y;
                return 1;
            }
            return gesture_is_ui_hold(engine->gesture) ? 1 : 0;
        }

        case AMOTION_EVENT_ACTION_POINTER_UP: {
            if (engine->gesture == GESTURE_PINCH && pointer_count == 2) {
                engine->gesture = GESTURE_BLOCKED;
                return 1;
            }
            return gesture_is_ui_hold(engine->gesture) ? 1 : 0;
        }

        case AMOTION_EVENT_ACTION_UP: {
#ifndef NDEBUG
            if (engine->gesture == GESTURE_FACTOR && engine->moved) {
                LOGI(
                    "factor drag completed: kind=%s index=%d",
                    engine->captured_factor_kind == FACTOR_POLE ? "pole" : "zero",
                    engine->captured_factor_index
                );
            }
#endif
            if (engine->gesture == GESTURE_SINGLE && !engine->moved) {
                float x = AMotionEvent_getX(event, 0);
                float y = AMotionEvent_getY(event, 0);
                if (engine->placement_kind == FACTOR_POLE) {
                    add_pole(engine, x, y);
                } else {
                    add_zero(engine, x, y);
                }
            } else if (
                engine->gesture == GESTURE_CLEAR_BUTTON &&
                clear_button_contains(
                    engine,
                    AMotionEvent_getX(event, 0),
                    AMotionEvent_getY(event, 0)
                )
            ) {
                clear_function(engine);
                LOGI("factors cleared");
            }
            engine->gesture = GESTURE_NONE;
            engine->moved = false;
            return 1;
        }

        case AMOTION_EVENT_ACTION_CANCEL:
            engine->gesture = GESTURE_NONE;
            engine->moved = false;
            return 1;

        default:
            return 0;
    }
}

static void handle_command(struct android_app *app, int32_t command) {
    struct engine *engine = app->userData;

    switch (command) {
        case APP_CMD_INIT_WINDOW:
            if (app->window != NULL && engine->display == EGL_NO_DISPLAY) {
                initialize_display(engine);
            }
            break;

        case APP_CMD_TERM_WINDOW:
            terminate_display(engine);
            break;

        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONTENT_RECT_CHANGED:
        case APP_CMD_CONFIG_CHANGED:
            update_surface_size(engine);
            break;

        case APP_CMD_GAINED_FOCUS:
            engine->dirty = true;
            break;

        default:
            break;
    }
}

void android_main(struct android_app *app) {
    struct engine engine = {
        .app = app,
        .display = EGL_NO_DISPLAY,
        .surface = EGL_NO_SURFACE,
        .context = EGL_NO_CONTEXT,
        .gesture = GESTURE_NONE,
        .dirty = true,
        .logged_first_frame = false
    };
    initialize_scene(&engine);

    app->userData = &engine;
    app->onAppCmd = handle_command;
    app->onInputEvent = handle_input;

    while (true) {
        int events = 0;
        struct android_poll_source *source = NULL;
        int timeout = engine.dirty && engine.display != EGL_NO_DISPLAY ? 0 : -1;
        int ident = ALooper_pollOnce(timeout, NULL, &events, (void **)&source);

        if (ident >= 0 && source != NULL) {
            source->process(app, source);
        }

        if (app->destroyRequested != 0) {
            terminate_display(&engine);
            return;
        }

        if (engine.dirty) {
            draw_frame(&engine);
        }
    }
}
