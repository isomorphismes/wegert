#ifndef WEGERT_FACTOR_SNAP_H
#define WEGERT_FACTOR_SNAP_H
#include "wegert_function.h"
int factor_snap_to_nearest(struct complex_value *point, struct complex_value factors[WEGERT_MAX_FACTORS],
                            int factor_count, float world_per_pixel, float touch_radius_pixels);
#endif
