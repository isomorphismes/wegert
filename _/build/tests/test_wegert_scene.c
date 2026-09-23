#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "../wegert_scene.h"

static int nearly_equal(float left, float right) {
    return fabsf(left - right) < 0.0001f;
}

int main(void) {
    struct wegert_scene scene;
    wegert_scene_initialize_default(&scene);
    wegert_scene_resize(&scene, 1000, 500);

    assert(scene.function.zero_count == 3);
    assert(nearly_equal(scene.view.center[0], 0.0f));
    assert(nearly_equal(scene.view.center[1], 0.0f));
    assert(nearly_equal(scene.view.half_height, 3.5f));
    assert(nearly_equal(wegert_scene_aspect(&scene), 2.0f));
    assert(nearly_equal(wegert_scene_world_units_per_pixel(&scene), 0.014f));

    float point[2] = {0.0f, 0.0f};
    assert(wegert_scene_screen_to_complex(&scene, 500.0f, 250.0f, point));
    assert(nearly_equal(point[0], 0.0f));
    assert(nearly_equal(point[1], 0.0f));

    assert(wegert_scene_screen_to_complex(&scene, 1000.0f, 0.0f, point));
    assert(nearly_equal(point[0], 7.0f));
    assert(nearly_equal(point[1], 3.5f));

    assert(wegert_scene_pan_by_pixels(&scene, 10.0f, -20.0f));
    assert(nearly_equal(scene.view.center[0], -0.14f));
    assert(nearly_equal(scene.view.center[1], -0.28f));

    puts("Wegert scene tests passed");
    return 0;
}
