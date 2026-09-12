#define WEGERT_MAX_FACTORS 64

__attribute__ ((visibility ("default")))
void
wegert_expand_roots_cartesian (const float *roots_cartesian,
                                int root_count,
                                double *coefficients_cartesian)
{
  double _Complex coefficients[WEGERT_MAX_FACTORS + 1];
  int index;

  if (root_count < 0)
    root_count = 0;
  if (root_count > WEGERT_MAX_FACTORS)
    root_count = WEGERT_MAX_FACTORS;

  for (index = 0; index <= WEGERT_MAX_FACTORS; ++index)
    coefficients[index] = __builtin_complex (0.0, 0.0);
  coefficients[0] = __builtin_complex (1.0, 0.0);

  for (int root_index = 0; root_index < root_count; ++root_index)
    {
      double _Complex next[WEGERT_MAX_FACTORS + 1];
      double _Complex root = __builtin_complex (
        (double) roots_cartesian[2 * root_index],
        (double) roots_cartesian[2 * root_index + 1]);

      for (index = 0; index <= WEGERT_MAX_FACTORS; ++index)
        next[index] = __builtin_complex (0.0, 0.0);

      for (int power = 0; power <= root_index; ++power)
        {
          next[power + 1] += coefficients[power];
          next[power] += -root * coefficients[power];
        }

      for (index = 0; index <= root_index + 1; ++index)
        coefficients[index] = next[index];
    }

  for (index = 0; index <= WEGERT_MAX_FACTORS; ++index)
    {
      coefficients_cartesian[2 * index] = __real__ coefficients[index];
      coefficients_cartesian[2 * index + 1] = __imag__ coefficients[index];
    }
}
