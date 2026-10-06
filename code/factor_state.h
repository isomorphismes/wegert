#ifndef WEGERT_FACTOR_STATE_H
#define WEGERT_FACTOR_STATE_H
#include "wegert_function.h"
enum factor_change { FACTOR_UNCHANGED, FACTOR_APPENDED, FACTOR_CANCELLED_OPPOSITE };
enum factor_change factor_insert_reduced(struct complex_value same_kind[WEGERT_MAX_FACTORS], int *same_kind_count,
                                         struct complex_value opposite_kind[WEGERT_MAX_FACTORS], int *opposite_kind_count,
                                         float real, float imag);
#endif
