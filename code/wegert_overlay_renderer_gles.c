#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wegert_overlay_renderer_gles.h"

#include "polynomial_text.h"
#include "wegert_gles.h"

static const char *POLYNOMIAL_OVERLAY_FRAGMENT_SHADER =
    "#version 300 es\n"
    "precision highp float;\n"
    "uniform vec2 u_resolution;\n"
    "uniform vec2 u_overlay_origin;\n"
    "uniform vec2 u_overlay_size;\n"
    "uniform sampler2D u_overlay;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    vec2 screen = vec2(gl_FragCoord.x, u_resolution.y - gl_FragCoord.y) - u_overlay_origin;\n"
    "    if (screen.x < 0.0 || screen.y < 0.0 || screen.x >= u_overlay_size.x || screen.y >= u_overlay_size.y) {\n"
    "        discard;\n"
    "    }\n"
    "    vec2 uv = (screen + vec2(0.5)) / u_overlay_size;\n"
    "    frag_color = texture(u_overlay, uv);\n"
    "}\n";

static int overlay_max_digit_run(const char *text) {
    int maximum = 0;
    int current = 0;
    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        if (*cursor >= '0' && *cursor <= '9') {
            current += 1;
            if (current > maximum) maximum = current;
        } else {
            current = 0;
        }
    }
    return maximum;
}

static int overlay_decode_glyph(const char *text, uint32_t *glyph) {
    const unsigned char first = (unsigned char)text[0];
    const unsigned char second = (unsigned char)text[1];
    if (first == 0xc3u && second == 0xb7u) {
        *glyph = 0x00f7u;
        return 2;
    }

    *glyph = (uint32_t)first;
    return 1;
}

static uint8_t overlay_glyph_row(uint32_t character, int row) {
    static const uint8_t digits[10][7] = {
        {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e},
        {0x04, 0x0c, 0x14, 0x04, 0x04, 0x04, 0x1f},
        {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f},
        {0x1e, 0x01, 0x01, 0x0e, 0x01, 0x01, 0x1e},
        {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02},
        {0x1f, 0x10, 0x10, 0x1e, 0x01, 0x01, 0x1e},
        {0x0e, 0x10, 0x10, 0x1e, 0x11, 0x11, 0x0e},
        {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
        {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e},
        {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x01, 0x0e}
    };

    if (character >= '0' && character <= '9') {
        return digits[character - '0'][row];
    }

    switch (character) {
        case 'a': { static const uint8_t glyph[7] = {0x00,0x00,0x0e,0x01,0x0f,0x11,0x0f}; return glyph[row]; }
        case 'c': { static const uint8_t glyph[7] = {0x00,0x00,0x0e,0x10,0x10,0x11,0x0e}; return glyph[row]; }
        case 'd': { static const uint8_t glyph[7] = {0x01,0x01,0x0f,0x11,0x11,0x11,0x0f}; return glyph[row]; }
        case 'e': { static const uint8_t glyph[7] = {0x00,0x00,0x0e,0x11,0x1f,0x10,0x0f}; return glyph[row]; }
        case 'f': { static const uint8_t glyph[7] = {0x06,0x08,0x08,0x1e,0x08,0x08,0x08}; return glyph[row]; }
        case 'h': { static const uint8_t glyph[7] = {0x10,0x10,0x1e,0x11,0x11,0x11,0x11}; return glyph[row]; }
        case 'i': { static const uint8_t glyph[7] = {0x04,0x00,0x0c,0x04,0x04,0x04,0x0e}; return glyph[row]; }
        case 'l': { static const uint8_t glyph[7] = {0x0c,0x04,0x04,0x04,0x04,0x04,0x0e}; return glyph[row]; }
        case 'n': { static const uint8_t glyph[7] = {0x00,0x00,0x1e,0x11,0x11,0x11,0x11}; return glyph[row]; }
        case 'o': { static const uint8_t glyph[7] = {0x00,0x00,0x0e,0x11,0x11,0x11,0x0e}; return glyph[row]; }
        case 'p': { static const uint8_t glyph[7] = {0x00,0x00,0x1e,0x11,0x1e,0x10,0x10}; return glyph[row]; }
        case 'r': { static const uint8_t glyph[7] = {0x00,0x00,0x16,0x19,0x10,0x10,0x10}; return glyph[row]; }
        case 't': { static const uint8_t glyph[7] = {0x08,0x08,0x1e,0x08,0x08,0x09,0x06}; return glyph[row]; }
        case 'u': { static const uint8_t glyph[7] = {0x00,0x00,0x11,0x11,0x11,0x13,0x0d}; return glyph[row]; }
        case 'w': { static const uint8_t glyph[7] = {0x00,0x00,0x11,0x11,0x15,0x15,0x0a}; return glyph[row]; }
        case 'x': { static const uint8_t glyph[7] = {0x00,0x00,0x11,0x0a,0x04,0x0a,0x11}; return glyph[row]; }
        case 'z': { static const uint8_t glyph[7] = {0x00,0x00,0x1f,0x02,0x04,0x08,0x1f}; return glyph[row]; }
        case '+': { static const uint8_t glyph[7] = {0x00,0x04,0x04,0x1f,0x04,0x04,0x00}; return glyph[row]; }
        case '-': { static const uint8_t glyph[7] = {0x00,0x00,0x00,0x1f,0x00,0x00,0x00}; return glyph[row]; }
        case '=': { static const uint8_t glyph[7] = {0x00,0x00,0x1f,0x00,0x1f,0x00,0x00}; return glyph[row]; }
        case '(': { static const uint8_t glyph[7] = {0x02,0x04,0x08,0x08,0x08,0x04,0x02}; return glyph[row]; }
        case ')': { static const uint8_t glyph[7] = {0x08,0x04,0x02,0x02,0x02,0x04,0x08}; return glyph[row]; }
        case '.': { static const uint8_t glyph[7] = {0x00,0x00,0x00,0x00,0x00,0x0c,0x0c}; return glyph[row]; }
        case ':': { static const uint8_t glyph[7] = {0x00,0x0c,0x0c,0x00,0x0c,0x0c,0x00}; return glyph[row]; }
        case '^': { static const uint8_t glyph[7] = {0x04,0x0a,0x11,0x00,0x00,0x00,0x00}; return glyph[row]; }
        case '/': { static const uint8_t glyph[7] = {0x01,0x02,0x04,0x04,0x08,0x10,0x00}; return glyph[row]; }
        case 0x00f7u: { static const uint8_t glyph[7] = {0x04,0x00,0x00,0x1f,0x00,0x00,0x04}; return glyph[row]; }
        default: return 0x00;
    }
}

static void overlay_set_pixel(
    uint8_t *pixels,
    int width,
    int height,
    int x,
    int y,
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    uint8_t alpha
) {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return;
    }
    size_t offset = ((size_t)y * (size_t)width + (size_t)x) * 4u;
    pixels[offset + 0u] = red;
    pixels[offset + 1u] = green;
    pixels[offset + 2u] = blue;
    pixels[offset + 3u] = alpha;
}

static void overlay_draw_glyph(
    uint8_t *pixels,
    int width,
    int height,
    int x,
    int y,
    int scale,
    uint32_t character
) {
    for (int row = 0; row < 7; ++row) {
        uint8_t bits = overlay_glyph_row(character, row);
        for (int column = 0; column < 5; ++column) {
            if ((bits & (1u << (4 - column))) == 0u) {
                continue;
            }
            for (int dy = 0; dy < scale; ++dy) {
                for (int dx = 0; dx < scale; ++dx) {
                    overlay_set_pixel(
                        pixels,
                        width,
                        height,
                        x + column * scale + dx,
                        y + row * scale + dy,
                        248,
                        244,
                        232,
                        255
                    );
                }
            }
        }
    }
}

static int overlay_measure_wrapped_lines(const char *text, int max_columns) {
    int lines = 1;
    int column = 0;
    const char *cursor = text;

    while (*cursor != '\0') {
        while (*cursor == ' ') cursor += 1;
        if (*cursor == '\0') break;

        int word_bytes = 0;
        int word_columns = 0;
        while (cursor[word_bytes] != '\0' && cursor[word_bytes] != ' ') {
            uint32_t glyph = 0u;
            word_bytes += overlay_decode_glyph(cursor + word_bytes, &glyph);
            word_columns += 1;
        }

        if (column == 0) {
            column = word_columns;
        } else if (column + 1 + word_columns <= max_columns) {
            column += 1 + word_columns;
        } else {
            lines += 1;
            column = word_columns;
        }
        cursor += word_bytes;
    }

    return lines;
}

static void overlay_draw_wrapped_text(
    uint8_t *pixels,
    int width,
    int height,
    const char *text,
    int scale,
    int padding,
    int max_columns
) {
    int character_advance = 6 * scale;
    int line_advance = 9 * scale;
    int x = padding;
    int y = padding;
    int column = 0;
    const char *cursor = text;

    while (*cursor != '\0') {
        while (*cursor == ' ') cursor += 1;
        if (*cursor == '\0') break;

        int word_bytes = 0;
        int word_columns = 0;
        while (cursor[word_bytes] != '\0' && cursor[word_bytes] != ' ') {
            uint32_t glyph = 0u;
            word_bytes += overlay_decode_glyph(cursor + word_bytes, &glyph);
            word_columns += 1;
        }

        if (column != 0 && column + 1 + word_columns <= max_columns) {
            x += character_advance;
            column += 1;
        } else if (column != 0) {
            x = padding;
            y += line_advance;
            column = 0;
        }

        int index = 0;
        while (index < word_bytes) {
            if (cursor[index] == '^') {
                int superscript_scale = scale > 1 ? scale - 1 : 1;
                int superscript_y = y - 2 * scale;
                column += 1;
                index += 1;
                while (
                    index < word_bytes &&
                    cursor[index] >= '0' &&
                    cursor[index] <= '9'
                ) {
                    overlay_draw_glyph(
                        pixels,
                        width,
                        height,
                        x,
                        superscript_y,
                        superscript_scale,
                        (uint32_t)(unsigned char)cursor[index]
                    );
                    x += 6 * superscript_scale;
                    column += 1;
                    index += 1;
                }
                continue;
            }

            uint32_t glyph = 0u;
            int glyph_bytes = overlay_decode_glyph(cursor + index, &glyph);
            overlay_draw_glyph(pixels, width, height, x, y, scale, glyph);
            x += character_advance;
            column += 1;
            index += glyph_bytes;
        }

        cursor += word_bytes;
    }
}

static void overlay_configure_texture(GLuint texture) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static int overlay_sanitize_density(int density_dpi) {
    if (density_dpi < 72 || density_dpi > 1000) {
        return 160;
    }
    return density_dpi;
}

static float clear_button_margin(int density_dpi) {
    int density = overlay_sanitize_density(density_dpi);
    return (float)((56 * density + 159) / 160);
}

static void clear_button_origin(
    const struct wegert_overlay_renderer_gles *renderer,
    const struct wegert_scene *scene,
    int density_dpi,
    float *x,
    float *y
) {
    float margin = clear_button_margin(density_dpi);
    *x = (float)(scene->width - renderer->clear_button_width) - margin;
    *y = (float)(scene->height - renderer->clear_button_height) - margin;
}

static bool overlay_initialize(
    struct wegert_overlay_renderer_gles *renderer,
    char *error,
    size_t error_capacity
) {
    if (renderer->program != 0) {
        return true;
    }
    if (renderer->unavailable) {
        return false;
    }

    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;
    if (!wegert_gles_compile_shader(
        GL_VERTEX_SHADER,
        WEGERT_FULLSCREEN_VERTEX_SHADER,
        &vertex_shader,
        error,
        error_capacity
    ) || !wegert_gles_compile_shader(
        GL_FRAGMENT_SHADER,
        POLYNOMIAL_OVERLAY_FRAGMENT_SHADER,
        &fragment_shader,
        error,
        error_capacity
    )) {
        if (vertex_shader != 0) glDeleteShader(vertex_shader);
        if (fragment_shader != 0) glDeleteShader(fragment_shader);
        renderer->unavailable = true;
        return false;
    }

    bool linked = wegert_gles_link_program(
        vertex_shader,
        fragment_shader,
        &renderer->program,
        error,
        error_capacity
    );
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    if (!linked) {
        renderer->unavailable = true;
        return false;
    }

    renderer->resolution_location =
        glGetUniformLocation(renderer->program, "u_resolution");
    renderer->origin_location =
        glGetUniformLocation(renderer->program, "u_overlay_origin");
    renderer->size_location =
        glGetUniformLocation(renderer->program, "u_overlay_size");
    renderer->sampler_location =
        glGetUniformLocation(renderer->program, "u_overlay");

    glGenTextures(1, &renderer->overlay_texture);
    overlay_configure_texture(renderer->overlay_texture);
    glGenTextures(1, &renderer->clear_button_texture);
    overlay_configure_texture(renderer->clear_button_texture);
    return true;
}

static bool overlay_rebuild_button_texture(
    GLuint texture,
    int density_dpi,
    const char *label,
    int *output_width,
    int *output_height
) {
    int density = overlay_sanitize_density(density_dpi);

    int scale = (density + 40) / 80;
    if (scale < 2) scale = 2;
    if (scale > 6) scale = 6;

    int label_characters = 0;
    while (label[label_characters] != '\0') label_characters += 1;
    int label_width = (6 * label_characters - 1) * scale;
    int minimum_height = (48 * density + 159) / 160;
    int horizontal_padding = (16 * density + 159) / 160;
    int width = label_width + 2 * horizontal_padding;
    int height = minimum_height;
    if (height < 11 * scale) height = 11 * scale;

    size_t pixel_count = (size_t)width * (size_t)height;
    uint8_t *pixels = malloc(pixel_count * 4u);
    if (pixels == NULL) {
        return false;
    }

    for (size_t index = 0; index < pixel_count; ++index) {
        pixels[index * 4u + 0u] = 18;
        pixels[index * 4u + 1u] = 18;
        pixels[index * 4u + 2u] = 18;
        pixels[index * 4u + 3u] = 230;
    }

    int border = scale > 2 ? 2 : 1;
    for (int x = 0; x < width; ++x) {
        for (int thickness = 0; thickness < border; ++thickness) {
            overlay_set_pixel(
                pixels, width, height, x, thickness,
                238, 235, 224, 255
            );
            overlay_set_pixel(
                pixels, width, height, x, height - 1 - thickness,
                238, 235, 224, 255
            );
        }
    }
    for (int y = 0; y < height; ++y) {
        for (int thickness = 0; thickness < border; ++thickness) {
            overlay_set_pixel(
                pixels, width, height, thickness, y,
                238, 235, 224, 255
            );
            overlay_set_pixel(
                pixels, width, height, width - 1 - thickness, y,
                238, 235, 224, 255
            );
        }
    }

    int x = (width - label_width) / 2;
    int y = (height - 7 * scale) / 2;
    for (int index = 0; label[index] != '\0'; ++index) {
        overlay_draw_glyph(
            pixels,
            width,
            height,
            x,
            y,
            scale,
            (uint32_t)(unsigned char)label[index]
        );
        x += 6 * scale;
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
    );
    glBindTexture(GL_TEXTURE_2D, 0);
    free(pixels);

    *output_width = width;
    *output_height = height;
    return true;
}

static bool clear_button_rebuild_texture(
    struct wegert_overlay_renderer_gles *renderer,
    int density_dpi
) {
    return overlay_rebuild_button_texture(
        renderer->clear_button_texture,
        density_dpi,
        "clear",
        &renderer->clear_button_width,
        &renderer->clear_button_height
    );
}

static bool polynomial_overlay_rebuild_texture(
    struct wegert_overlay_renderer_gles *renderer,
    const struct wegert_scene *scene
) {
    if (scene->width <= 64 || scene->height <= 64) {
        return false;
    }

    char text[4096];
    polynomial_text_format_function(
        scene->function.zeros,
        scene->function.zero_count,
        scene->function.poles,
        scene->function.pole_count,
        text,
        sizeof(text)
    );

    int width = scene->width - 32;
    if (width > 1280) width = 1280;
    int scale = width >= 900 ? 3 : 2;
    int max_number_digits = overlay_max_digit_run(text);
    if (max_number_digits >= 4 && scale > 2) scale = 2;
    if (max_number_digits >= 7) scale = 1;
    int padding = 4 * scale;
    int character_advance = 6 * scale;
    int line_advance = 9 * scale;
    int max_columns = (width - 2 * padding) / character_advance;
    if (max_columns < 12) max_columns = 12;

    int lines = overlay_measure_wrapped_lines(text, max_columns);
    int height = 2 * padding + 7 * scale + (lines - 1) * line_advance;
    if (height > scene->height - 32) {
        height = scene->height - 32;
    }

    size_t pixel_count = (size_t)width * (size_t)height;
    uint8_t *pixels = malloc(pixel_count * 4u);
    if (pixels == NULL) {
        return false;
    }

    for (size_t index = 0; index < pixel_count; ++index) {
        pixels[index * 4u + 0u] = 18;
        pixels[index * 4u + 1u] = 18;
        pixels[index * 4u + 2u] = 18;
        pixels[index * 4u + 3u] = 214;
    }

    for (int x = 0; x < width; ++x) {
        for (int thickness = 0; thickness < 2; ++thickness) {
            overlay_set_pixel(
                pixels, width, height, x, thickness,
                238, 235, 224, 240
            );
            overlay_set_pixel(
                pixels, width, height, x, height - 1 - thickness,
                238, 235, 224, 240
            );
        }
    }
    for (int y = 0; y < height; ++y) {
        for (int thickness = 0; thickness < 2; ++thickness) {
            overlay_set_pixel(
                pixels, width, height, thickness, y,
                238, 235, 224, 240
            );
            overlay_set_pixel(
                pixels, width, height, width - 1 - thickness, y,
                238, 235, 224, 240
            );
        }
    }

    overlay_draw_wrapped_text(
        pixels,
        width,
        height,
        text,
        scale,
        padding,
        max_columns
    );

    glBindTexture(GL_TEXTURE_2D, renderer->overlay_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
    );
    glBindTexture(GL_TEXTURE_2D, 0);
    free(pixels);

    renderer->overlay_width = width;
    renderer->overlay_height = height;
    renderer->dirty = false;
    return true;
}

static void overlay_draw_texture(
    const struct wegert_overlay_renderer_gles *renderer,
    GLuint fullscreen_vao,
    GLuint texture,
    int width,
    int height,
    float x,
    float y
) {
    glUniform2f(renderer->origin_location, x, y);
    glUniform2f(renderer->size_location, (float)width, (float)height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(renderer->sampler_location, 0);
    glBindVertexArray(fullscreen_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void wegert_overlay_renderer_gles_mark_dirty(
    struct wegert_overlay_renderer_gles *renderer
) {
    renderer->dirty = true;
}

bool wegert_overlay_renderer_gles_draw(
    struct wegert_overlay_renderer_gles *renderer,
    const struct wegert_scene *scene,
    int density_dpi,
    GLuint fullscreen_vao,
    char *error,
    size_t error_capacity
) {
    if (!overlay_initialize(renderer, error, error_capacity)) {
        return false;
    }
    if (renderer->dirty && !polynomial_overlay_rebuild_texture(renderer, scene)) {
        return false;
    }
    if (
        (renderer->clear_button_width <= 0 ||
         renderer->clear_button_height <= 0) &&
        !clear_button_rebuild_texture(renderer, density_dpi)
    ) {
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(renderer->program);
    glUniform2f(
        renderer->resolution_location,
        (float)scene->width,
        (float)scene->height
    );

    float clear_button_x = 0.0f;
    float clear_button_y = 0.0f;
    clear_button_origin(
        renderer,
        scene,
        density_dpi,
        &clear_button_x,
        &clear_button_y
    );

    overlay_draw_texture(
        renderer,
        fullscreen_vao,
        renderer->overlay_texture,
        renderer->overlay_width,
        renderer->overlay_height,
        16.0f,
        16.0f
    );
    overlay_draw_texture(
        renderer,
        fullscreen_vao,
        renderer->clear_button_texture,
        renderer->clear_button_width,
        renderer->clear_button_height,
        clear_button_x,
        clear_button_y
    );

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
    return true;
}

void wegert_overlay_renderer_gles_destroy(
    struct wegert_overlay_renderer_gles *renderer
) {
    if (renderer->overlay_texture != 0) {
        glDeleteTextures(1, &renderer->overlay_texture);
        renderer->overlay_texture = 0;
    }
    if (renderer->clear_button_texture != 0) {
        glDeleteTextures(1, &renderer->clear_button_texture);
        renderer->clear_button_texture = 0;
    }
    if (renderer->program != 0) {
        glDeleteProgram(renderer->program);
        renderer->program = 0;
    }
    renderer->overlay_width = 0;
    renderer->overlay_height = 0;
    renderer->clear_button_width = 0;
    renderer->clear_button_height = 0;
    renderer->unavailable = false;
    renderer->dirty = true;
}

bool wegert_overlay_renderer_gles_formula_contains(
    const struct wegert_overlay_renderer_gles *renderer,
    float x,
    float y
) {
    return renderer->overlay_width > 0 && renderer->overlay_height > 0 &&
        x >= 16.0f && x < 16.0f + (float)renderer->overlay_width &&
        y >= 16.0f && y < 16.0f + (float)renderer->overlay_height;
}

bool wegert_overlay_renderer_gles_clear_button_contains(
    const struct wegert_overlay_renderer_gles *renderer,
    const struct wegert_scene *scene,
    int density_dpi,
    float x,
    float y
) {
    float left = 0.0f;
    float top = 0.0f;
    clear_button_origin(renderer, scene, density_dpi, &left, &top);
    return renderer->clear_button_width > 0 &&
        renderer->clear_button_height > 0 &&
        x >= left &&
        x < left + (float)renderer->clear_button_width &&
        y >= top &&
        y < top + (float)renderer->clear_button_height;
}

bool wegert_overlay_renderer_gles_clear_button_center(
    const struct wegert_overlay_renderer_gles *renderer,
    const struct wegert_scene *scene,
    int density_dpi,
    float *x,
    float *y
) {
    if (
        renderer->clear_button_width <= 0 ||
        renderer->clear_button_height <= 0
    ) {
        return false;
    }

    float left = 0.0f;
    float top = 0.0f;
    clear_button_origin(renderer, scene, density_dpi, &left, &top);
    *x = left + 0.5f * (float)renderer->clear_button_width;
    *y = top + 0.5f * (float)renderer->clear_button_height;
    return true;
}
