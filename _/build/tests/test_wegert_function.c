#include <assert.h>
#include <stdio.h>

#include "../wegert_function.h"

int main(void) {
    struct wegert_function function;
    wegert_function_initialize_default(&function);

    assert(function.zero_count == 3);
    assert(function.pole_count == 0);
    assert(function.zeros[0][0] == 1.0f && function.zeros[0][1] == 0.0f);
    assert(function.zeros[1][0] == 2.0f && function.zeros[1][1] == 0.0f);
    assert(function.zeros[2][0] == 5.0f && function.zeros[2][1] == 0.0f);

    function.pole_count = 2;
    wegert_function_clear(&function);
    assert(function.zero_count == 0);
    assert(function.pole_count == 0);

    puts("Wegert function tests passed");
    return 0;
}
