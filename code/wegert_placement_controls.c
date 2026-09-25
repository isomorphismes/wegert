#include "wegert_placement_controls.h"

#include <math.h>

bool wegert_placement_controls_layout(
    int width,
    int height,
    struct wegert_placement_controls *controls
) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    float radius = 0.065f * fminf((float)width, (float)height);
    if (radius < 36.0f) radius = 36.0f;
    if (radius > 56.0f) radius = 56.0f;

    float margin = fmaxf(18.0f, radius * 0.38f);

    controls->radius = radius;
    controls->zero_center[0] = margin + radius;
    controls->zero_center[1] = (float)height - margin - radius;
    controls->pole_center[0] =
        controls->zero_center[0] + 2.0f * radius + margin * 0.55f;
    controls->pole_center[1] = controls->zero_center[1];
    return true;
}

bool wegert_placement_controls_hit(
    const struct wegert_placement_controls *controls,
    float x,
    float y,
    enum factor_kind *kind
) {
    if (hypotf(
        x - controls->zero_center[0],
        y - controls->zero_center[1]
    ) <= controls->radius) {
        *kind = FACTOR_ZERO;
        return true;
    }

    if (hypotf(
        x - controls->pole_center[0],
        y - controls->pole_center[1]
    ) <= controls->radius) {
        *kind = FACTOR_POLE;
        return true;
    }

    return false;
}
