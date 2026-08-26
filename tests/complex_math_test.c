#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../complex_math.h"

static void require_close(double actual, double expected, const char *what) {
    if (fabs(actual - expected) > 1.0e-9) {
        fprintf(stderr, "%s: got %.17g expected %.17g\n", what, actual, expected);
        exit(1);
    }
}

int main(void) {
    double coefficients[130];
    const float real_roots[] = {1.0f, 0.0f, 2.0f, 0.0f, 5.0f, 0.0f};
    wegert_expand_roots_cartesian(real_roots, 3, coefficients);
    require_close(coefficients[0], -10.0, "real c0");
    require_close(coefficients[1], 0.0, "imag c0");
    require_close(coefficients[2], 17.0, "real c1");
    require_close(coefficients[4], -8.0, "real c2");
    require_close(coefficients[6], 1.0, "real c3");

    const float complex_roots[] = {1.0f, 2.0f, -3.0f, 0.5f};
    wegert_expand_roots_cartesian(complex_roots, 2, coefficients);
    require_close(coefficients[0], -4.0, "complex c0 real");
    require_close(coefficients[1], -5.5, "complex c0 imag");
    require_close(coefficients[2], 2.0, "complex c1 real");
    require_close(coefficients[3], -2.5, "complex c1 imag");
    require_close(coefficients[4], 1.0, "complex c2 real");
    require_close(coefficients[5], 0.0, "complex c2 imag");
    return 0;
}
