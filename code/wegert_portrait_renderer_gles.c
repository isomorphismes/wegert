#include "wegert_portrait_renderer_gles.h"

#include <string.h>

#include "wegert_gles.h"

bool wegert_portrait_renderer_gles_initialize(
    struct wegert_portrait_renderer_gles *renderer,
    const char *fragment_shader_source,
    char *error,
    size_t error_capacity
) {
    memset(renderer, 0, sizeof(*renderer));

    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;
    if (!wegert_gles_compile_shader(
        GL_VERTEX_SHADER,
        WEGERT_FULLSCREEN_VERTEX_SHADER,
        &vertex_shader,
        error,
        error_capacity
    )) {
        return false;
    }
    if (!wegert_gles_compile_shader(
        GL_FRAGMENT_SHADER,
        fragment_shader_source,
        &fragment_shader,
        error,
        error_capacity
    )) {
        glDeleteShader(vertex_shader);
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
        return false;
    }

    renderer->center_location =
        glGetUniformLocation(renderer->program, "u_center");
    renderer->half_height_location =
        glGetUniformLocation(renderer->program, "u_half_height");
    renderer->aspect_location =
        glGetUniformLocation(renderer->program, "u_aspect");
    renderer->resolution_location =
        glGetUniformLocation(renderer->program, "u_resolution");
    renderer->zero_count_location =
        glGetUniformLocation(renderer->program, "u_zero_count");
    renderer->pole_count_location =
        glGetUniformLocation(renderer->program, "u_pole_count");
    renderer->zeros_location =
        glGetUniformLocation(renderer->program, "u_zeros[0]");
    renderer->poles_location =
        glGetUniformLocation(renderer->program, "u_poles[0]");

    if (!wegert_gles_create_fullscreen_triangle(
        &renderer->vao,
        &renderer->vbo
    )) {
        if (error != NULL && error_capacity > 0u) {
            const char message[] =
                "could not create portrait fullscreen triangle";
            size_t index = 0u;
            while (
                index + 1u < error_capacity &&
                index + 1u < sizeof(message)
            ) {
                error[index] = message[index];
                ++index;
            }
            error[index] = '\0';
        }
        wegert_portrait_renderer_gles_destroy(renderer);
        return false;
    }

    return true;
}

void wegert_portrait_renderer_gles_destroy(
    struct wegert_portrait_renderer_gles *renderer
) {
    wegert_gles_destroy_fullscreen_triangle(&renderer->vao, &renderer->vbo);
    if (renderer->program != 0) {
        glDeleteProgram(renderer->program);
        renderer->program = 0;
    }
}

bool wegert_portrait_renderer_gles_render_frame(
    const struct wegert_portrait_renderer_gles *renderer,
    const struct wegert_scene *scene
) {
    if (
        renderer->program == 0 ||
        scene->width <= 0 ||
        scene->height <= 0
    ) {
        return false;
    }

    glViewport(0, 0, scene->width, scene->height);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glUseProgram(renderer->program);
    glUniform2f(
        renderer->center_location,
        scene->view.center[0],
        scene->view.center[1]
    );
    glUniform1f(renderer->half_height_location, scene->view.half_height);
    glUniform1f(renderer->aspect_location, wegert_scene_aspect(scene));
    glUniform2f(
        renderer->resolution_location,
        (float)scene->width,
        (float)scene->height
    );
    glUniform1i(renderer->zero_count_location, scene->function.zero_count);
    glUniform1i(renderer->pole_count_location, scene->function.pole_count);
    glUniform2fv(
        renderer->zeros_location,
        WEGERT_MAX_FACTORS,
        &scene->function.zeros[0][0]
    );
    glUniform2fv(
        renderer->poles_location,
        WEGERT_MAX_FACTORS,
        &scene->function.poles[0][0]
    );

    glBindVertexArray(renderer->vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    return true;
}
