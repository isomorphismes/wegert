#include "wegert_function.h"

#include <string.h>

void wegert_function_initialize_default(struct wegert_function *function) {
    memset(function, 0, sizeof(*function));

    function->zero_count = 3;
    function->zeros[0][0] = 1.0f;
    function->zeros[0][1] = 0.0f;
    function->zeros[1][0] = 2.0f;
    function->zeros[1][1] = 0.0f;
    function->zeros[2][0] = 5.0f;
    function->zeros[2][1] = 0.0f;
}

void wegert_function_clear(struct wegert_function *function) {
    function->zero_count = 0;
    function->pole_count = 0;
}
