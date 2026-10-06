#include "factor_state.h"

#include "wegert_function.h"



static int factor_find_exact(
    struct complex_value factors[WEGERT_MAX_FACTORS],
    int factor_count,
    float real,
    float imag
) {
    for (int index = 0; index < factor_count; ++index) {
        if (factors[index].real == real && factors[index].imaginary == imag) {
            return index;
        }
    }
    return -1;
}

static void factor_remove_at(
    struct complex_value factors[WEGERT_MAX_FACTORS],
    int *factor_count,
    int removed_index
) {
    for (int index = removed_index; index + 1 < *factor_count; ++index) {
        factors[index].real = factors[index + 1].real;
        factors[index].imaginary = factors[index + 1].imaginary;
    }
    *factor_count -= 1;
}

enum factor_change factor_insert_reduced(
    struct complex_value same_kind[WEGERT_MAX_FACTORS],
    int *same_kind_count,
    struct complex_value opposite_kind[WEGERT_MAX_FACTORS],
    int *opposite_kind_count,
    float real,
    float imag
) {
    int opposite_index = factor_find_exact(
        opposite_kind,
        *opposite_kind_count,
        real,
        imag
    );
    if (opposite_index >= 0) {
        factor_remove_at(opposite_kind, opposite_kind_count, opposite_index);
        return FACTOR_CANCELLED_OPPOSITE;
    }

    if (*same_kind_count >= WEGERT_MAX_FACTORS) {
        return FACTOR_UNCHANGED;
    }

    same_kind[*same_kind_count].real = real;
    same_kind[*same_kind_count].imaginary = imag;
    *same_kind_count += 1;
    return FACTOR_APPENDED;
}
