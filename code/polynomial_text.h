#ifndef WEGERT_POLYNOMIAL_TEXT_H
#define WEGERT_POLYNOMIAL_TEXT_H
#include "wegert_function.h"
void polynomial_text_format_function(const struct complex_value zeros[WEGERT_MAX_FACTORS], int zero_count,
                                      const struct complex_value poles[WEGERT_MAX_FACTORS], int pole_count,
                                      char *output, size_t capacity);
#endif
