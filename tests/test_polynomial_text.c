#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MAX_FACTORS 64

#include "../app/src/main/cpp/polynomial_text.h"

static void test_polynomial_without_poles(void) {
    const float zeros[MAX_FACTORS][2] = {{1.0f, 0.0f}};
    const float poles[MAX_FACTORS][2] = {{0.0f, 0.0f}};
    char output[256];

    polynomial_text_format_function(zeros, 1, poles, 0, output, sizeof(output));

    assert(strcmp(output, "z - 1") == 0);
}

static void test_division_sign_and_separate_denominator_factors(void) {
    const float zeros[MAX_FACTORS][2] = {{1.0f, 0.0f}};
    const float poles[MAX_FACTORS][2] = {{2.0f, 0.0f}, {0.0f, 1.0f}};
    char output[256];

    polynomial_text_format_function(zeros, 1, poles, 2, output, sizeof(output));

    assert(strcmp(output, "(z - 1) ÷(z-2) ÷(z-i)") == 0);
}

int main(void) {
    test_polynomial_without_poles();
    test_division_sign_and_separate_denominator_factors();
    puts("polynomial text tests passed");
    return 0;
}
