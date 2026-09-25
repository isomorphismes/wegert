#include "wegert_gles.h"

#include <stdio.h>

const char WEGERT_FULLSCREEN_VERTEX_SHADER[] =
    "#version 300 es\n"
    "precision highp float;\n"
    "layout(location = 0) in vec2 a_position;\n"
    "out vec2 v_ndc;\n"
    "void main() {\n"
    "    v_ndc = a_position;\n"
    "    gl_Position = vec4(a_position, 0.0, 1.0);\n"
    "}\n";

static void wegert_gles_set_error(
    char *error,
    size_t error_capacity,
    const char *prefix,
    const char *details
) {
    if (error == NULL || error_capacity == 0u) {
        return;
    }
    if (details != NULL && details[0] != '\0') {
        snprintf(error, error_capacity, "%s: %s", prefix, details);
    } else {
        snprintf(error, error_capacity, "%s", prefix);
    }
}

bool wegert_gles_compile_shader(
    GLenum type,
    const char *source,
    GLuint *output_shader,
    char *error,
    size_t error_capacity
) {
    *output_shader = 0;
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        wegert_gles_set_error(
            error,
            error_capacity,
            "shader compilation failed",
            "glCreateShader returned 0"
        );
        return false;
    }

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        *output_shader = shader;
        return true;
    }

    char details[1536] = {0};
    GLsizei written = 0;
    glGetShaderInfoLog(shader, (GLsizei)sizeof(details), &written, details);
    (void)written;
    wegert_gles_set_error(
        error,
        error_capacity,
        "shader compilation failed",
        details
    );
    glDeleteShader(shader);
    return false;
}

bool wegert_gles_link_program(
    GLuint vertex_shader,
    GLuint fragment_shader,
    GLuint *output_program,
    char *error,
    size_t error_capacity
) {
    *output_program = 0;
    GLuint program = glCreateProgram();
    if (program == 0) {
        wegert_gles_set_error(
            error,
            error_capacity,
            "program link failed",
            "glCreateProgram returned 0"
        );
        return false;
    }

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        *output_program = program;
        return true;
    }

    char details[1536] = {0};
    GLsizei written = 0;
    glGetProgramInfoLog(program, (GLsizei)sizeof(details), &written, details);
    (void)written;
    wegert_gles_set_error(error, error_capacity, "program link failed", details);
    glDeleteProgram(program);
    return false;
}

bool wegert_gles_create_fullscreen_triangle(GLuint *output_vao, GLuint *output_vbo) {
    static const GLfloat fullscreen_triangle[] = {
        -1.0f, -1.0f,
         3.0f, -1.0f,
        -1.0f,  3.0f
    };

    *output_vao = 0;
    *output_vbo = 0;

    glGenVertexArrays(1, output_vao);
    glBindVertexArray(*output_vao);

    glGenBuffers(1, output_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, *output_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(fullscreen_triangle),
        fullscreen_triangle,
        GL_STATIC_DRAW
    );
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * (GLsizei)sizeof(GLfloat),
        (const void *)0
    );
    glEnableVertexAttribArray(0);

    return *output_vao != 0 && *output_vbo != 0;
}

void wegert_gles_destroy_fullscreen_triangle(GLuint *vao, GLuint *vbo) {
    if (*vbo != 0) {
        glDeleteBuffers(1, vbo);
        *vbo = 0;
    }
    if (*vao != 0) {
        glDeleteVertexArrays(1, vao);
        *vao = 0;
    }
}
