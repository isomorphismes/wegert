#ifndef WEGERT_PLACEMENT_CONTROLS_H
#define WEGERT_PLACEMENT_CONTROLS_H

#include <stdbool.h>

#include "factor_drag.h"

struct wegert_placement_controls {
    float radius;
    float zero_center[2];
    float pole_center[2];
};

bool wegert_placement_controls_layout(
    int width,
    int height,
    struct wegert_placement_controls *controls
);

bool wegert_placement_controls_hit(
    const struct wegert_placement_controls *controls,
    float x,
    float y,
    enum factor_kind *kind
);

#endif
