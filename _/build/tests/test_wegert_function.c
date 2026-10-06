#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "../wegert_function.h"

int main(void) {
    struct wegert_function function;
    wegert_function_initialize_default(&function);

    assert(function.zero_count == 3);
    assert(function.pole_count == 0);

    function.zeros[0]=(struct complex_value){1,0}; function.zero_count=1;
    function.poles[0]=(struct complex_value){0,1}; function.pole_count=1;
    struct wegert_function saved=function;
    struct complex_value value=wegert_function_evaluate(&function,(struct complex_value){2,3});
    /* (1+3i)/(2+2i) = 1 + i/2. */
    assert(fabsf(value.real-1.0f)<1e-6f && fabsf(value.imaginary-0.5f)<1e-6f);
    float packed[2]; complex_values_pack(function.poles,1,packed);
    assert(packed[0]==0 && packed[1]==1);
    assert(memcmp(&saved,&function,sizeof(function))==0);
    wegert_function_initialize_default(&function);
    assert(function.zeros[0].real == 1.0f && function.zeros[0].imaginary == 0.0f);
    assert(function.zeros[1].real == 2.0f && function.zeros[1].imaginary == 0.0f);
    assert(function.zeros[2].real == 5.0f && function.zeros[2].imaginary == 0.0f);

    function.pole_count = 2;
    wegert_function_clear(&function);
    assert(function.zero_count == 0);
    assert(function.pole_count == 0);

    puts("Wegert function tests passed");
    return 0;
}
