#ifndef WEGERT_POLYNOMIAL_OVERLAY_H
#define WEGERT_POLYNOMIAL_OVERLAY_H

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct overlay_complex {
    double real;
    double imag;
};

static const char *POLYNOMIAL_OVERLAY_FRAGMENT_SHADER =
    "#version 300 es\n"
    "precision highp float;\n"
    "uniform vec2 u_resolution;\n"
    "uniform vec2 u_overlay_size;\n"
    "uniform sampler2D u_overlay;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    vec2 screen = vec2(gl_FragCoord.x, u_resolution.y - gl_FragCoord.y) - vec2(16.0);\n"
    "    if (screen.x < 0.0 || screen.y < 0.0 || screen.x >= u_overlay_size.x || screen.y >= u_overlay_size.y) {\n"
    "        discard;\n"
    "    }\n"
    "    vec2 uv = (screen + vec2(0.5)) / u_overlay_size;\n"
    "    frag_color = texture(u_overlay, uv);\n"
    "}\n";

static bool overlay_near_zero(double value) {
    return fabs(value) < 1.0e-6;
}

static bool overlay_near_one(double value) {
    return fabs(value - 1.0) < 1.0e-6;
}

static void overlay_append(char *buffer, size_t capacity, size_t *used, const char *format, ...) {
    if (*used + 1u >= capacity) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    int written = vsnprintf(buffer + *used, capacity - *used, format, arguments);
    va_end(arguments);

    if (written < 0) {
        return;
    }

    size_t remaining = capacity - *used;
    if ((size_t)written >= remaining) {
        *used = capacity - 1u;
    } else {
        *used += (size_t)written;
    }
}

static void overlay_format_number(double value, char *output, size_t capacity) {
    if (overlay_near_zero(value)) {
        snprintf(output, capacity, "0");
        return;
    }

    double absolute = fabs(value);
    double rounded = round(value);
    if (fabs(value - rounded) < 1.0e-6 * fmax(1.0, absolute)) {
        snprintf(output, capacity, "%.0f", rounded);
        return;
    }

    if (absolute >= 1000000.0 || absolute < 0.01) {
        snprintf(output, capacity, "%.2e", value);
        return;
    }

    snprintf(output, capacity, "%.2f", value);
    size_t length = strlen(output);
    while (length > 0u && output[length - 1u] == '0') {
        output[--length] = '\0';
    }
    if (length > 0u && output[length - 1u] == '.') {
        output[--length] = '\0';
    }
}

static void overlay_expand_roots(
    const float roots[MAX_FACTORS][2],
    int root_count,
    struct overlay_complex coefficients[MAX_FACTORS + 1]
) {
    for (int index = 0; index <= MAX_FACTORS; ++index) {
        coefficients[index].real = 0.0;
        coefficients[index].imag = 0.0;
    }
    coefficients[0].real = 1.0;

    int degree = 0;
    for (int root_index = 0; root_index < root_count; ++root_index) {
        struct overlay_complex next[MAX_FACTORS + 1] = {{0.0, 0.0}};
        double root_real = roots[root_index][0];
        double root_imag = roots[root_index][1];

        for (int power = 0; power <= degree; ++power) {
            double coefficient_real = coefficients[power].real;
            double coefficient_imag = coefficients[power].imag;

            next[power + 1].real += coefficient_real;
            next[power + 1].imag += coefficient_imag;

            next[power].real += -root_real * coefficient_real + root_imag * coefficient_imag;
            next[power].imag += -root_real * coefficient_imag - root_imag * coefficient_real;
        }

        degree += 1;
        for (int power = 0; power <= degree; ++power) {
            coefficients[power] = next[power];
        }
    }
}

static void overlay_append_complex_magnitude(
    char *output,
    size_t capacity,
    size_t *used,
    struct overlay_complex value
) {
    bool has_real = !overlay_near_zero(value.real);
    bool has_imag = !overlay_near_zero(value.imag);
    char number[64];

    if (has_real && has_imag) {
        overlay_format_number(value.real, number, sizeof(number));
        overlay_append(output, capacity, used, "(%s", number);

        double imag_magnitude = fabs(value.imag);
        overlay_append(output, capacity, used, value.imag < 0.0 ? "-" : "+");
        if (!overlay_near_one(imag_magnitude)) {
            overlay_format_number(imag_magnitude, number, sizeof(number));
            overlay_append(output, capacity, used, "%s", number);
        }
        overlay_append(output, capacity, used, "i)");
        return;
    }

    if (has_imag) {
        double imag_magnitude = fabs(value.imag);
        if (!overlay_near_one(imag_magnitude)) {
            overlay_format_number(imag_magnitude, number, sizeof(number));
            overlay_append(output, capacity, used, "%s", number);
        }
        overlay_append(output, capacity, used, "i");
        return;
    }

    overlay_format_number(fabs(value.real), number, sizeof(number));
    overlay_append(output, capacity, used, "%s", number);
}

static void overlay_format_polynomial(
    const struct overlay_complex coefficients[MAX_FACTORS + 1],
    int degree,
    char *output,
    size_t capacity
) {
    size_t used = 0u;
    output[0] = '\0';
    bool first_term = true;

    for (int power = degree; power >= 0; --power) {
        struct overlay_complex coefficient = coefficients[power];
        if (overlay_near_zero(coefficient.real) && overlay_near_zero(coefficient.imag)) {
            continue;
        }

        if (overlay_near_zero(coefficient.imag)) {
            bool negative = coefficient.real < 0.0;
            double magnitude = fabs(coefficient.real);

            if (first_term) {
                if (negative) overlay_append(output, capacity, &used, "-");
            } else {
                overlay_append(output, capacity, &used, negative ? " - " : " + ");
            }

            if (power == 0 || !overlay_near_one(magnitude)) {
                char number[64];
                overlay_format_number(magnitude, number, sizeof(number));
                overlay_append(output, capacity, &used, "%s", number);
            }
        } else {
            bool negative = coefficient.real < -1.0e-6 ||
                (overlay_near_zero(coefficient.real) && coefficient.imag < 0.0);
            struct overlay_complex magnitude = coefficient;
            if (negative) {
                magnitude.real = -magnitude.real;
                magnitude.imag = -magnitude.imag;
            }

            if (first_term) {
                if (negative) overlay_append(output, capacity, &used, "-");
            } else {
                overlay_append(output, capacity, &used, negative ? " - " : " + ");
            }
            overlay_append_complex_magnitude(output, capacity, &used, magnitude);
        }

        if (power > 0) {
            overlay_append(output, capacity, &used, "z");
            if (power > 1) {
                overlay_append(output, capacity, &used, "^%d", power);
            }
        }

        first_term = false;
    }

    if (first_term) {
        overlay_append(output, capacity, &used, "0");
    }
}

static void polynomial_overlay_format_function(const struct engine *engine, char *output, size_t capacity) {
    struct overlay_complex numerator_coefficients[MAX_FACTORS + 1];
    struct overlay_complex denominator_coefficients[MAX_FACTORS + 1];
    char numerator[2048];
    char denominator[2048];

    overlay_expand_roots(engine->zeros, engine->zero_count, numerator_coefficients);
    overlay_format_polynomial(numerator_coefficients, engine->zero_count, numerator, sizeof(numerator));

    if (engine->pole_count == 0) {
        snprintf(output, capacity, "expanded: f(z) = %s", numerator);
        return;
    }

    overlay_expand_roots(engine->poles, engine->pole_count, denominator_coefficients);
    overlay_format_polynomial(denominator_coefficients, engine->pole_count, denominator, sizeof(denominator));
    snprintf(output, capacity, "expanded: f(z) = (%s) / (%s)", numerator, denominator);
}

static uint8_t overlay_glyph_row(char character, int row) {
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
        case 'd': { static const uint8_t glyph[7] = {0x01,0x01,0x0f,0x11,0x11,0x11,0x0f}; return glyph[row]; }
        case 'e': { static const uint8_t glyph[7] = {0x00,0x00,0x0e,0x11,0x1f,0x10,0x0f}; return glyph[row]; }
        case 'f': { static const uint8_t glyph[7] = {0x06,0x08,0x08,0x1e,0x08,0x08,0x08}; return glyph[row]; }
        case 'i': { static const uint8_t glyph[7] = {0x04,0x00,0x0c,0x04,0x04,0x04,0x0e}; return glyph[row]; }
        case 'n': { static const uint8_t glyph[7] = {0x00,0x00,0x1e,0x11,0x11,0x11,0x11}; return glyph[row]; }
        case 'p': { static const uint8_t glyph[7] = {0x00,0x00,0x1e,0x11,0x1e,0x10,0x10}; return glyph[row]; }
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
    char character
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

        int word_length = 0;
        while (cursor[word_length] != '\0' && cursor[word_length] != ' ') {
            word_length += 1;
        }

        if (column == 0) {
            column = word_length;
        } else if (column + 1 + word_length <= max_columns) {
            column += 1 + word_length;
        } else {
            lines += 1;
            column = word_length;
        }
        cursor += word_length;
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

        int word_length = 0;
        while (cursor[word_length] != '\0' && cursor[word_length] != ' ') {
            word_length += 1;
        }

        if (column != 0 && column + 1 + word_length <= max_columns) {
            x += character_advance;
            column += 1;
        } else if (column != 0) {
            x = padding;
            y += line_advance;
            column = 0;
        }

        for (int index = 0; index < word_length; ++index) {
            overlay_draw_glyph(pixels, width, height, x, y, scale, cursor[index]);
            x += character_advance;
            column += 1;
        }

        cursor += word_length;
    }
}

static bool polynomial_overlay_initialize(struct engine *engine) {
    if (engine->overlay_program != 0) {
        return true;
    }
    if (engine->overlay_unavailable) {
        return false;
    }

    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, VERTEX_SHADER);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, POLYNOMIAL_OVERLAY_FRAGMENT_SHADER);
    if (vertex_shader == 0 || fragment_shader == 0) {
        if (vertex_shader != 0) glDeleteShader(vertex_shader);
        if (fragment_shader != 0) glDeleteShader(fragment_shader);
        engine->overlay_unavailable = true;
        return false;
    }

    engine->overlay_program = link_program(vertex_shader, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    if (engine->overlay_program == 0) {
        engine->overlay_unavailable = true;
        return false;
    }

    engine->overlay_resolution_location = glGetUniformLocation(engine->overlay_program, "u_resolution");
    engine->overlay_size_location = glGetUniformLocation(engine->overlay_program, "u_overlay_size");
    engine->overlay_sampler_location = glGetUniformLocation(engine->overlay_program, "u_overlay");

    glGenTextures(1, &engine->overlay_texture);
    glBindTexture(GL_TEXTURE_2D, engine->overlay_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

static bool polynomial_overlay_rebuild_texture(struct engine *engine) {
    if (engine->width <= 64 || engine->height <= 64) {
        return false;
    }

    char text[4096];
    polynomial_overlay_format_function(engine, text, sizeof(text));

    int width = engine->width - 32;
    if (width > 1280) width = 1280;
    int scale = width >= 900 ? 3 : 2;
    int padding = 4 * scale;
    int character_advance = 6 * scale;
    int line_advance = 9 * scale;
    int max_columns = (width - 2 * padding) / character_advance;
    if (max_columns < 12) max_columns = 12;

    int lines = overlay_measure_wrapped_lines(text, max_columns);
    int height = 2 * padding + 7 * scale + (lines - 1) * line_advance;
    if (height > engine->height - 32) {
        height = engine->height - 32;
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
            overlay_set_pixel(pixels, width, height, x, thickness, 238, 235, 224, 240);
            overlay_set_pixel(pixels, width, height, x, height - 1 - thickness, 238, 235, 224, 240);
        }
    }
    for (int y = 0; y < height; ++y) {
        for (int thickness = 0; thickness < 2; ++thickness) {
            overlay_set_pixel(pixels, width, height, thickness, y, 238, 235, 224, 240);
            overlay_set_pixel(pixels, width, height, width - 1 - thickness, y, 238, 235, 224, 240);
        }
    }

    overlay_draw_wrapped_text(pixels, width, height, text, scale, padding, max_columns);

    glBindTexture(GL_TEXTURE_2D, engine->overlay_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    free(pixels);

    engine->overlay_width = width;
    engine->overlay_height = height;
    engine->overlay_dirty = false;
    return true;
}

static void polynomial_overlay_draw(struct engine *engine) {
    if (!polynomial_overlay_initialize(engine)) {
        return;
    }
    if (engine->overlay_dirty && !polynomial_overlay_rebuild_texture(engine)) {
        return;
    }
    if (engine->overlay_width <= 0 || engine->overlay_height <= 0) {
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(engine->overlay_program);
    glUniform2f(engine->overlay_resolution_location, (float)engine->width, (float)engine->height);
    glUniform2f(engine->overlay_size_location, (float)engine->overlay_width, (float)engine->overlay_height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, engine->overlay_texture);
    glUniform1i(engine->overlay_sampler_location, 0);
    glBindVertexArray(engine->vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}

static void polynomial_overlay_destroy(struct engine *engine) {
    if (engine->overlay_texture != 0) {
        glDeleteTextures(1, &engine->overlay_texture);
        engine->overlay_texture = 0;
    }
    if (engine->overlay_program != 0) {
        glDeleteProgram(engine->overlay_program);
        engine->overlay_program = 0;
    }
    engine->overlay_width = 0;
    engine->overlay_height = 0;
    engine->overlay_unavailable = false;
    engine->overlay_dirty = true;
}

static bool polynomial_overlay_contains(const struct engine *engine, float x, float y) {
    return engine->overlay_width > 0 && engine->overlay_height > 0 &&
        x >= 16.0f && x < 16.0f + (float)engine->overlay_width &&
        y >= 16.0f && y < 16.0f + (float)engine->overlay_height;
}

#endif
