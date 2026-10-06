#include "wegert_function.h"

#include <string.h>

void complex_values_pack(const struct complex_value *values, size_t count, float *cartesian) {
    for (size_t i=0; i<count; ++i) {
        cartesian[2*i]=values[i].real;
        cartesian[2*i+1]=values[i].imaginary;
    }
}
struct complex_value complex_difference(struct complex_value a, struct complex_value b) {
    return (struct complex_value){a.real-b.real, a.imaginary-b.imaginary};
}
struct complex_value complex_product(struct complex_value a, struct complex_value b) {
    return (struct complex_value){a.real*b.real-a.imaginary*b.imaginary, a.real*b.imaginary+a.imaginary*b.real};
}
struct complex_value complex_quotient(struct complex_value a, struct complex_value b) {
    float norm=b.real*b.real+b.imaginary*b.imaginary;
    return (struct complex_value){(a.real*b.real+a.imaginary*b.imaginary)/norm,
                                 (a.imaginary*b.real-a.real*b.imaginary)/norm};
}
static struct complex_value evaluate_factor_product(const struct complex_value *roots, int count,
                                                     struct complex_value point) {
    struct complex_value value={1,0};
    for (int i=0; i<count; ++i) value=complex_product(value,complex_difference(point,roots[i]));
    return value;
}
struct complex_value wegert_function_evaluate(const struct wegert_function *function, struct complex_value point) {
    struct complex_value numerator=evaluate_factor_product(function->zeros,function->zero_count,point);
    struct complex_value denominator=evaluate_factor_product(function->poles,function->pole_count,point);
    return complex_quotient(numerator,denominator);
}

void wegert_function_initialize_default(struct wegert_function *function) {
    memset(function, 0, sizeof(*function));

    function->zero_count = 3;
    function->zeros[0].real = 1.0f;
    function->zeros[0].imaginary = 0.0f;
    function->zeros[1].real = 2.0f;
    function->zeros[1].imaginary = 0.0f;
    function->zeros[2].real = 5.0f;
    function->zeros[2].imaginary = 0.0f;
}

void wegert_function_clear(struct wegert_function *function) {
    function->zero_count = 0;
    function->pole_count = 0;
}
