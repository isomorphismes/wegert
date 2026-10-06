#include "../factor_state.h"
void invalid_factors(float roots[WEGERT_MAX_FACTORS][2],int *count)
{
    (void)factor_insert_reduced(roots,count,roots,count,1,0);
}
