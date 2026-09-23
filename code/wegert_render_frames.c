#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "wegert_offscreen_gles.h"
#include "wegert_scene.h"

static char *read_text_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    long length = ftell(file);
    if (length < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    char *text = malloc((size_t)length + 1u);
    if (text == NULL) {
        fclose(file);
        return NULL;
    }
    if (fread(text, 1u, (size_t)length, file) != (size_t)length) {
        free(text);
        fclose(file);
        return NULL;
    }
    text[length] = '\0';
    fclose(file);
    return text;
}

static int parse_int(const char *text, int *value) {
    char *end = NULL;
    errno = 0;
    long parsed = strtol(text, &end, 10);
    if (
        errno != 0 ||
        end == text ||
        *end != '\0' ||
        parsed < 1 ||
        parsed > 16384
    ) {
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

static int parse_float(const char *text, float *value) {
    char *end = NULL;
    errno = 0;
    float parsed = strtof(text, &end);
    if (errno != 0 || end == text || *end != '\0') {
        return 0;
    }
    *value = parsed;
    return 1;
}

static int read_function(struct wegert_function *function) {
    int zero_count = 0;
    int pole_count = 0;
    int read = scanf("%d %d", &zero_count, &pole_count);
    if (read == EOF) {
        return 0;
    }
    if (
        read != 2 ||
        zero_count < 0 ||
        zero_count > WEGERT_MAX_FACTORS ||
        pole_count < 0 ||
        pole_count > WEGERT_MAX_FACTORS
    ) {
        return -1;
    }

    function->zero_count = zero_count;
    function->pole_count = pole_count;

    for (int index = 0; index < zero_count; ++index) {
        if (scanf(
            "%f %f",
            &function->zeros[index][0],
            &function->zeros[index][1]
        ) != 2) {
            return -1;
        }
    }
    for (int index = 0; index < pole_count; ++index) {
        if (scanf(
            "%f %f",
            &function->poles[index][0],
            &function->poles[index][1]
        ) != 2) {
            return -1;
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 7) {
        fprintf(
            stderr,
            "usage: %s FRAGMENT_SHADER WIDTH HEIGHT CENTER_X CENTER_Y HALF_HEIGHT\n"
            "stdin: zero_count pole_count then zero/pole real-imaginary pairs per frame\n",
            argv[0]
        );
        return 2;
    }

    int width = 0;
    int height = 0;
    float center_x = 0.0f;
    float center_y = 0.0f;
    float half_height = 0.0f;
    if (
        !parse_int(argv[2], &width) ||
        !parse_int(argv[3], &height) ||
        !parse_float(argv[4], &center_x) ||
        !parse_float(argv[5], &center_y) ||
        !parse_float(argv[6], &half_height) ||
        half_height <= 0.0f
    ) {
        fprintf(stderr, "invalid frame geometry\n");
        return 2;
    }

    char *fragment_shader_source = read_text_file(argv[1]);
    if (fragment_shader_source == NULL) {
        fprintf(stderr, "could not read fragment shader: %s\n", argv[1]);
        return 1;
    }

    char error[2048] = {0};
    struct wegert_offscreen_gles offscreen;
    if (!wegert_offscreen_gles_initialize(
        &offscreen,
        width,
        height,
        fragment_shader_source,
        error,
        sizeof(error)
    )) {
        fprintf(stderr, "%s\n", error);
        free(fragment_shader_source);
        return 1;
    }
    free(fragment_shader_source);

    struct wegert_scene scene;
    wegert_scene_initialize_default(&scene);
    wegert_scene_resize(&scene, width, height);
    scene.view.center[0] = center_x;
    scene.view.center[1] = center_y;
    scene.view.half_height = half_height;

    size_t rgba_bytes = (size_t)width * (size_t)height * 4u;
    size_t rgb_bytes = (size_t)width * (size_t)height * 3u;
    uint8_t *rgba = malloc(rgba_bytes);
    uint8_t *rgb = malloc(rgb_bytes);
    if (rgba == NULL || rgb == NULL) {
        fprintf(stderr, "could not allocate frame buffers\n");
        free(rgba);
        free(rgb);
        wegert_offscreen_gles_destroy(&offscreen);
        return 1;
    }

    for (;;) {
        int status = read_function(&scene.function);
        if (status == 0) {
            break;
        }
        if (status < 0) {
            fprintf(stderr, "invalid frame description on stdin\n");
            free(rgba);
            free(rgb);
            wegert_offscreen_gles_destroy(&offscreen);
            return 2;
        }

        if (!wegert_offscreen_gles_render_rgba(
            &offscreen,
            &scene,
            rgba,
            rgba_bytes,
            error,
            sizeof(error)
        )) {
            fprintf(stderr, "%s\n", error);
            free(rgba);
            free(rgb);
            wegert_offscreen_gles_destroy(&offscreen);
            return 1;
        }

        size_t pixel_count = (size_t)width * (size_t)height;
        for (size_t pixel = 0; pixel < pixel_count; ++pixel) {
            rgb[3u * pixel + 0u] = rgba[4u * pixel + 0u];
            rgb[3u * pixel + 1u] = rgba[4u * pixel + 1u];
            rgb[3u * pixel + 2u] = rgba[4u * pixel + 2u];
        }

        if (fwrite(rgb, 1u, rgb_bytes, stdout) != rgb_bytes) {
            fprintf(stderr, "could not write RGB frame\n");
            free(rgba);
            free(rgb);
            wegert_offscreen_gles_destroy(&offscreen);
            return 1;
        }
    }

    free(rgba);
    free(rgb);
    wegert_offscreen_gles_destroy(&offscreen);
    return 0;
}
