#ifndef WEGERT_POLYNOMIAL_TEXT_H
#define WEGERT_POLYNOMIAL_TEXT_H

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "complex_math.h"

struct polynomial_text_complex {
    double real;
    double imag;
};

static bool polynomial_text_near_zero(double value) {
    return fabs(value) < 1.0e-6;
}

static bool polynomial_text_near_one(double value) {
    return fabs(value - 1.0) < 1.0e-6;
}

static void polynomial_text_append(
    char *buffer,
    size_t capacity,
    size_t *used,
    const char *format,
    ...
) {
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

static void polynomial_text_format_number(double value, char *output, size_t capacity) {
    double rounded = round(value);
    if (polynomial_text_near_zero(rounded)) {
        snprintf(output, capacity, "0");
        return;
    }
    snprintf(output, capacity, "%.0f", rounded);
}

static void polynomial_text_expand_roots(
    const float roots[MAX_FACTORS][2],
    int root_count,
    struct polynomial_text_complex coefficients[MAX_FACTORS + 1]
) {
    double coefficients_cartesian[(MAX_FACTORS + 1) * 2];
    wegert_expand_roots_cartesian(&roots[0][0], root_count, coefficients_cartesian);

    for (int index = 0; index <= MAX_FACTORS; ++index) {
        coefficients[index].real = coefficients_cartesian[2 * index];
        coefficients[index].imag = coefficients_cartesian[2 * index + 1];
    }
}

static void polynomial_text_append_complex_magnitude(
    char *output,
    size_t capacity,
    size_t *used,
    struct polynomial_text_complex value
) {
    bool has_real = !polynomial_text_near_zero(value.real);
    bool has_imag = !polynomial_text_near_zero(value.imag);
    char number[64];

    if (has_real && has_imag) {
        polynomial_text_format_number(value.real, number, sizeof(number));
        polynomial_text_append(output, capacity, used, "(%s", number);

        double imag_magnitude = fabs(value.imag);
        polynomial_text_append(output, capacity, used, value.imag < 0.0 ? "-" : "+");
        if (!polynomial_text_near_one(imag_magnitude)) {
            polynomial_text_format_number(imag_magnitude, number, sizeof(number));
            polynomial_text_append(output, capacity, used, "%s", number);
        }
        polynomial_text_append(output, capacity, used, "i)");
        return;
    }

    if (has_imag) {
        double imag_magnitude = fabs(value.imag);
        if (!polynomial_text_near_one(imag_magnitude)) {
            polynomial_text_format_number(imag_magnitude, number, sizeof(number));
            polynomial_text_append(output, capacity, used, "%s", number);
        }
        polynomial_text_append(output, capacity, used, "i");
        return;
    }

    polynomial_text_format_number(fabs(value.real), number, sizeof(number));
    polynomial_text_append(output, capacity, used, "%s", number);
}

static void polynomial_text_format_polynomial(
    const struct polynomial_text_complex coefficients[MAX_FACTORS + 1],
    int degree,
    char *output,
    size_t capacity
) {
    size_t used = 0u;
    output[0] = '\0';
    bool first_term = true;

    for (int power = degree; power >= 0; --power) {
        struct polynomial_text_complex coefficient = coefficients[power];
        coefficient.real = round(coefficient.real);
        coefficient.imag = round(coefficient.imag);
        if (
            polynomial_text_near_zero(coefficient.real) &&
            polynomial_text_near_zero(coefficient.imag)
        ) {
            continue;
        }

        if (polynomial_text_near_zero(coefficient.imag)) {
            bool negative = coefficient.real < 0.0;
            double magnitude = fabs(coefficient.real);

            if (first_term) {
                if (negative) polynomial_text_append(output, capacity, &used, "-");
            } else {
                polynomial_text_append(output, capacity, &used, negative ? " - " : " + ");
            }

            if (power == 0 || !polynomial_text_near_one(magnitude)) {
                char number[64];
                polynomial_text_format_number(magnitude, number, sizeof(number));
                polynomial_text_append(output, capacity, &used, "%s", number);
            }
        } else {
            bool negative = coefficient.real < -1.0e-6 ||
                (polynomial_text_near_zero(coefficient.real) && coefficient.imag < 0.0);
            struct polynomial_text_complex magnitude = coefficient;
            if (negative) {
                magnitude.real = -magnitude.real;
                magnitude.imag = -magnitude.imag;
            }

            if (first_term) {
                if (negative) polynomial_text_append(output, capacity, &used, "-");
            } else {
                polynomial_text_append(output, capacity, &used, negative ? " - " : " + ");
            }
            polynomial_text_append_complex_magnitude(output, capacity, &used, magnitude);
        }

        if (power > 0) {
            polynomial_text_append(output, capacity, &used, "z");
            if (power > 1) {
                polynomial_text_append(output, capacity, &used, "^%d", power);
            }
        }

        first_term = false;
    }

    if (first_term) {
        polynomial_text_append(output, capacity, &used, "0");
    }
}

static void polynomial_text_append_factor(
    char *output,
    size_t capacity,
    size_t *used,
    const float root[2]
) {
    double real = round((double)root[0]);
    double imag = round((double)root[1]);
    char number[64];

    polynomial_text_append(output, capacity, used, "(z");

    if (!polynomial_text_near_zero(real)) {
        polynomial_text_format_number(fabs(real), number, sizeof(number));
        polynomial_text_append(output, capacity, used, real > 0.0 ? "-%s" : "+%s", number);
    }

    if (!polynomial_text_near_zero(imag)) {
        double magnitude = fabs(imag);
        polynomial_text_append(output, capacity, used, imag > 0.0 ? "-" : "+");
        if (!polynomial_text_near_one(magnitude)) {
            polynomial_text_format_number(magnitude, number, sizeof(number));
            polynomial_text_append(output, capacity, used, "%s", number);
        }
        polynomial_text_append(output, capacity, used, "i");
    }

    polynomial_text_append(output, capacity, used, ")");
}

static void polynomial_text_format_function(
    const float zeros[MAX_FACTORS][2],
    int zero_count,
    const float poles[MAX_FACTORS][2],
    int pole_count,
    char *output,
    size_t capacity
) {
    struct polynomial_text_complex numerator_coefficients[MAX_FACTORS + 1];
    char numerator[2048];

    polynomial_text_expand_roots(zeros, zero_count, numerator_coefficients);
    polynomial_text_format_polynomial(
        numerator_coefficients,
        zero_count,
        numerator,
        sizeof(numerator)
    );

    if (pole_count == 0) {
        snprintf(output, capacity, "%s", numerator);
        return;
    }

    size_t used = 0u;
    output[0] = '\0';
    polynomial_text_append(output, capacity, &used, "(%s)", numerator);
    for (int index = 0; index < pole_count; ++index) {
        polynomial_text_append(output, capacity, &used, " ÷");
        polynomial_text_append_factor(output, capacity, &used, poles[index]);
    }
}

#endif
