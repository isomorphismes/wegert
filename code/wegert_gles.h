#ifndef WEGERT_GLES_H
#define WEGERT_GLES_H

#include <stdbool.h>
#include <stddef.h>

#include <GLES3/gl3.h>

extern const char WEGERT_FULLSCREEN_VERTEX_SHADER[];

bool wegert_gles_compile_shader(
    GLenum type,
    const char *source,
    GLuint *output_shader,
    char *error,
    size_t error_capacity
);

bool wegert_gles_link_program(
    GLuint vertex_shader,
    GLuint fragment_shader,
    GLuint *output_program,
    char *error,
    size_t error_capacity
);

bool wegert_gles_create_fullscreen_triangle(GLuint *output_vao, GLuint *output_vbo);

void wegert_gles_destroy_fullscreen_triangle(GLuint *vao, GLuint *vbo);

#endif
