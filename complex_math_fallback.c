#include "complex_math.h"

#define WEGERT_MAX_FACTORS 64

void wegert_expand_roots_cartesian(
    const float *roots_cartesian,
    int root_count,
    double *coefficients_cartesian
) {
    double coefficients[WEGERT_MAX_FACTORS + 1][2] = {{0.0, 0.0}};
    coefficients[0][0] = 1.0;

    if (root_count < 0) root_count = 0;
    if (root_count > WEGERT_MAX_FACTORS) root_count = WEGERT_MAX_FACTORS;

    for (int root_index = 0; root_index < root_count; ++root_index) {
        double next[WEGERT_MAX_FACTORS + 1][2] = {{0.0, 0.0}};
        double root_real = roots_cartesian[2 * root_index];
        double root_imag = roots_cartesian[2 * root_index + 1];

        for (int power = 0; power <= root_index; ++power) {
            double coefficient_real = coefficients[power][0];
            double coefficient_imag = coefficients[power][1];

            next[power + 1][0] += coefficient_real;
            next[power + 1][1] += coefficient_imag;
            next[power][0] += -root_real * coefficient_real + root_imag * coefficient_imag;
            next[power][1] += -root_real * coefficient_imag - root_imag * coefficient_real;
        }

        for (int power = 0; power <= root_index + 1; ++power) {
            coefficients[power][0] = next[power][0];
            coefficients[power][1] = next[power][1];
        }
    }

    for (int index = 0; index <= WEGERT_MAX_FACTORS; ++index) {
        coefficients_cartesian[2 * index] = coefficients[index][0];
        coefficients_cartesian[2 * index + 1] = coefficients[index][1];
    }
}
