#include <android/asset_manager.h>
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

#define LOG_TAG "Wegert"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define MAX_FACTORS 16

static const char *VERTEX_SHADER =
    "#version 300 es\n"
    "precision highp float;\n"
    "const vec2 positions[3] = vec2[3](\n"
    "    vec2(-1.0, -1.0),\n"
    "    vec2( 3.0, -1.0),\n"
    "    vec2(-1.0,  3.0)\n"
    ");\n"
    "out vec2 v_ndc;\n"
    "void main() {\n"
    "    vec2 position = positions[gl_VertexID];\n"
    "    v_ndc = position;\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "}\n";

enum gesture_kind {
    GESTURE_NONE,
    GESTURE_SINGLE,
    GESTURE_PINCH,
    GESTURE_BLOCKED
};

struct engine {
    struct android_app *app;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;

    GLuint program;
    GLuint vao;
    GLint center_location;
    GLint half_height_location;
    GLint aspect_location;
    GLint resolution_location;
    GLint zero_count_location;
    GLint pole_count_location;
    GLint zeros_location;
    GLint poles_location;

    float center[2];
    float half_height;
    float zeros[MAX_FACTORS][2];
    float poles[MAX_FACTORS][2];
    int zero_count;
    int pole_count;

    enum gesture_kind gesture;
    bool moved;
    float down_x;
    float down_y;
    float last_x;
    float last_y;
    float pinch_last_distance;
    float pinch_last_mid_x;
    float pinch_last_mid_y;

    bool dirty;
    bool logged_first_frame;
};

static void reset_function(struct engine *engine) {
    engine->center[0] = 0.0f;
    engine->center[1] = 0.0f;
    engine->half_height = 3.5f;

    engine->zero_count = 3;
    engine->zeros[0][0] = 1.0f;
    engine->zeros[0][1] = 0.0f;
    engine->zeros[1][0] = 2.0f;
    engine->zeros[1][1] = 0.0f;
    engine->zeros[2][0] = 5.0f;
    engine->zeros[2][1] = 0.0f;
    engine->pole_count = 0;
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

static GLuint compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    char *log = length > 0 ? malloc((size_t)length) : NULL;
    if (log != NULL) {
        glGetShaderInfoLog(shader, length, NULL, log);
        LOGE("shader compilation failed: %s", log);
        free(log);
    } else {
        LOGE("shader compilation failed");
    }
    glDeleteShader(shader);
    return 0;
}

static GLuint link_program(GLuint vertex_shader, GLuint fragment_shader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return program;
    }

    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    char *log = length > 0 ? malloc((size_t)length) : NULL;
    if (log != NULL) {
        glGetProgramInfoLog(program, length, NULL, log);
        LOGE("program link failed: %s", log);
        free(log);
    } else {
        LOGE("program link failed");
    }
    glDeleteProgram(program);
    return 0;
}

static bool create_renderer(struct engine *engine) {
    char *fragment_source = load_asset_text(engine->app->activity->assetManager, "wegert.frag");
    if (fragment_source == NULL) {
        return false;
    }

    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, VERTEX_SHADER);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    free(fragment_source);

    if (vertex_shader == 0 || fragment_shader == 0) {
        if (vertex_shader != 0) glDeleteShader(vertex_shader);
        if (fragment_shader != 0) glDeleteShader(fragment_shader);
        return false;
    }

    engine->program = link_program(vertex_shader, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    if (engine->program == 0) {
        return false;
    }

    engine->center_location = glGetUniformLocation(engine->program, "u_center");
    engine->half_height_location = glGetUniformLocation(engine->program, "u_half_height");
    engine->aspect_location = glGetUniformLocation(engine->program, "u_aspect");
    engine->resolution_location = glGetUniformLocation(engine->program, "u_resolution");
    engine->zero_count_location = glGetUniformLocation(engine->program, "u_zero_count");
    engine->pole_count_location = glGetUniformLocation(engine->program, "u_pole_count");
    engine->zeros_location = glGetUniformLocation(engine->program, "u_zeros[0]");
    engine->poles_location = glGetUniformLocation(engine->program, "u_poles[0]");

    glGenVertexArrays(1, &engine->vao);
    glBindVertexArray(engine->vao);
    glDisable(GL_DEPTH_TEST);

    LOGI("renderer ready: GL_VERSION=%s GL_RENDERER=%s program=%u vao=%u uniforms=%d,%d,%d,%d,%d,%d,%d,%d",
         glGetString(GL_VERSION), glGetString(GL_RENDERER), engine->program, engine->vao,
         engine->center_location, engine->half_height_location, engine->aspect_location,
         engine->resolution_location, engine->zero_count_location, engine->pole_count_location,
         engine->zeros_location, engine->poles_location);
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
    eglQuerySurface(display, surface, EGL_WIDTH, &engine->width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &engine->height);
    LOGI("EGL surface ready: %dx%d", engine->width, engine->height);

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

    glViewport(0, 0, engine->width, engine->height);
    engine->dirty = true;
    return true;
}

static void terminate_display(struct engine *engine) {
    if (engine->display == EGL_NO_DISPLAY) {
        return;
    }

    if (engine->program != 0) {
        glDeleteProgram(engine->program);
        engine->program = 0;
    }
    if (engine->vao != 0) {
        glDeleteVertexArrays(1, &engine->vao);
        engine->vao = 0;
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
    eglQuerySurface(engine->display, engine->surface, EGL_WIDTH, &engine->width);
    eglQuerySurface(engine->display, engine->surface, EGL_HEIGHT, &engine->height);
    glViewport(0, 0, engine->width, engine->height);
    engine->dirty = true;
}

static void draw_frame(struct engine *engine) {
    if (engine->display == EGL_NO_DISPLAY || engine->program == 0 || engine->width <= 0 || engine->height <= 0) {
        return;
    }

    float aspect = (float)engine->width / (float)engine->height;

    glUseProgram(engine->program);
    glUniform2f(engine->center_location, engine->center[0], engine->center[1]);
    glUniform1f(engine->half_height_location, engine->half_height);
    glUniform1f(engine->aspect_location, aspect);
    glUniform2f(engine->resolution_location, (float)engine->width, (float)engine->height);
    glUniform1i(engine->zero_count_location, engine->zero_count);
    glUniform1i(engine->pole_count_location, engine->pole_count);
    glUniform2fv(engine->zeros_location, MAX_FACTORS, &engine->zeros[0][0]);
    glUniform2fv(engine->poles_location, MAX_FACTORS, &engine->poles[0][0]);

    glBindVertexArray(engine->vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    if (!engine->logged_first_frame) {
        GLubyte pixel[4] = {0, 0, 0, 0};
        glReadPixels(engine->width / 2, engine->height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
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
    float width = (float)engine->width;
    float height = (float)engine->height;
    float aspect = width / height;
    float ndc_x = 2.0f * x / width - 1.0f;
    float ndc_y = 1.0f - 2.0f * y / height;

    output[0] = engine->center[0] + ndc_x * engine->half_height * aspect;
    output[1] = engine->center[1] + ndc_y * engine->half_height;
}

static void pan_by_pixels(struct engine *engine, float delta_x, float delta_y) {
    if (engine->width <= 0 || engine->height <= 0) {
        return;
    }
    float aspect = (float)engine->width / (float)engine->height;
    engine->center[0] -= 2.0f * delta_x * engine->half_height * aspect / (float)engine->width;
    engine->center[1] += 2.0f * delta_y * engine->half_height / (float)engine->height;
    engine->dirty = true;
}

static void add_zero(struct engine *engine, float x, float y) {
    if (engine->zero_count >= MAX_FACTORS || engine->width <= 0 || engine->height <= 0) {
        return;
    }
    screen_to_complex(engine, x, y, engine->zeros[engine->zero_count]);
    engine->zero_count += 1;
    engine->dirty = true;
}

static void add_pole(struct engine *engine, float x, float y) {
    if (engine->pole_count >= MAX_FACTORS || engine->width <= 0 || engine->height <= 0) {
        return;
    }
    screen_to_complex(engine, x, y, engine->poles[engine->pole_count]);
    engine->pole_count += 1;
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
            engine->gesture = GESTURE_SINGLE;
            engine->moved = false;
            engine->down_x = AMotionEvent_getX(event, 0);
            engine->down_y = AMotionEvent_getY(event, 0);
            engine->last_x = engine->down_x;
            engine->last_y = engine->down_y;
            return 1;
        }

        case AMOTION_EVENT_ACTION_POINTER_DOWN: {
            if (pointer_count >= 3) {
                reset_function(engine);
                engine->gesture = GESTURE_BLOCKED;
                return 1;
            }
            if (pointer_count == 2) {
                engine->gesture = GESTURE_PINCH;
                engine->moved = false;
                engine->pinch_last_distance = pointer_distance(event);
                pointer_midpoint(event, &engine->pinch_last_mid_x, &engine->pinch_last_mid_y);
                return 1;
            }
            return 0;
        }

        case AMOTION_EVENT_ACTION_MOVE: {
            if (engine->gesture == GESTURE_SINGLE && pointer_count == 1) {
                float x = AMotionEvent_getX(event, 0);
                float y = AMotionEvent_getY(event, 0);
                float from_down_x = x - engine->down_x;
                float from_down_y = y - engine->down_y;
                if (hypotf(from_down_x, from_down_y) > 12.0f) {
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

                float midpoint_motion = hypotf(
                    midpoint_x - engine->pinch_last_mid_x,
                    midpoint_y - engine->pinch_last_mid_y
                );
                if (midpoint_motion > 4.0f || fabsf(distance - engine->pinch_last_distance) > 4.0f) {
                    engine->moved = true;
                }

                pan_by_pixels(
                    engine,
                    midpoint_x - engine->pinch_last_mid_x,
                    midpoint_y - engine->pinch_last_mid_y
                );

                if (distance > 1.0f && engine->pinch_last_distance > 1.0f) {
                    engine->half_height *= engine->pinch_last_distance / distance;
                    if (engine->half_height < 0.01f) engine->half_height = 0.01f;
                    if (engine->half_height > 100000.0f) engine->half_height = 100000.0f;
                    engine->dirty = true;
                }

                engine->pinch_last_distance = distance;
                engine->pinch_last_mid_x = midpoint_x;
                engine->pinch_last_mid_y = midpoint_y;
                return 1;
            }
            return engine->gesture == GESTURE_BLOCKED ? 1 : 0;
        }

        case AMOTION_EVENT_ACTION_POINTER_UP: {
            if (engine->gesture == GESTURE_PINCH && pointer_count == 2) {
                if (!engine->moved) {
                    float midpoint_x = 0.0f;
                    float midpoint_y = 0.0f;
                    pointer_midpoint(event, &midpoint_x, &midpoint_y);
                    add_pole(engine, midpoint_x, midpoint_y);
                }
                engine->gesture = GESTURE_BLOCKED;
                return 1;
            }
            return engine->gesture == GESTURE_BLOCKED ? 1 : 0;
        }

        case AMOTION_EVENT_ACTION_UP: {
            if (engine->gesture == GESTURE_SINGLE && !engine->moved) {
                add_zero(engine, AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));
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
    reset_function(&engine);

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
