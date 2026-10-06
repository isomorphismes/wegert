#include "complex_math.h"
#define WEGERT_MAX_FACTORS 64
struct polynomial_coefficient { double real, imaginary; };

static void multiply_by_linear_factor(struct polynomial_coefficient *coefficients,
                                      int degree, struct polynomial_coefficient root)
{
    struct polynomial_coefficient next[WEGERT_MAX_FACTORS + 1]={{0,0}};
    for (int power=0; power<=degree; ++power) {
        struct polynomial_coefficient coefficient=coefficients[power];
        next[power+1].real+=coefficient.real;
        next[power+1].imaginary+=coefficient.imaginary;
        next[power].real+=-root.real*coefficient.real+root.imaginary*coefficient.imaginary;
        next[power].imaginary+=-root.real*coefficient.imaginary-root.imaginary*coefficient.real;
    }
    for (int power=0; power<=degree+1; ++power) coefficients[power]=next[power];
}

/* Adapter for separately compiled objects; the mathematical working state is
 * a polynomial with named complex coefficients, not a Cartesian buffer. */
__attribute__((visibility("default")))
void wegert_expand_roots_cartesian(const float *roots_cartesian, int root_count,
                                  double *coefficients_cartesian)
{
    struct polynomial_coefficient coefficients[WEGERT_MAX_FACTORS+1]={{1,0}};
    if (root_count<0) root_count=0;
    if (root_count>WEGERT_MAX_FACTORS) root_count=WEGERT_MAX_FACTORS;
    for (int index=0; index<root_count; ++index) {
        struct polynomial_coefficient root={roots_cartesian[2*index],roots_cartesian[2*index+1]};
        multiply_by_linear_factor(coefficients,index,root);
    }
    for (int index=0; index<=WEGERT_MAX_FACTORS; ++index) {
        coefficients_cartesian[2*index]=coefficients[index].real;
        coefficients_cartesian[2*index+1]=coefficients[index].imaginary;
    }
}
