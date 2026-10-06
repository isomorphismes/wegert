#ifndef WEGERT_COMPLEX_VALUE_H
#define WEGERT_COMPLEX_VALUE_H
#include <stddef.h>

struct complex_value { float real, imaginary; };
/* Cartesian buffers belong to foreign/GPU interfaces, never to the model. */
void complex_values_pack(const struct complex_value *values, size_t count, float *cartesian);
struct complex_value complex_difference(struct complex_value a, struct complex_value b);
struct complex_value complex_product(struct complex_value a, struct complex_value b);
struct complex_value complex_quotient(struct complex_value a, struct complex_value b);
#endif
