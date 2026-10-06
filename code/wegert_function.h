#ifndef WEGERT_FUNCTION_H
#define WEGERT_FUNCTION_H

#define WEGERT_MAX_FACTORS 64
#include "complex_value.h"

struct wegert_function {
    struct complex_value zeros[WEGERT_MAX_FACTORS];
    struct complex_value poles[WEGERT_MAX_FACTORS];
    int zero_count;
    int pole_count;
};

void wegert_function_initialize_default(struct wegert_function *function);
void wegert_function_clear(struct wegert_function *function);
struct complex_value wegert_function_evaluate(const struct wegert_function *function, struct complex_value point);

#endif
