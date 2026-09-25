#ifndef WEGERT_FUNCTION_H
#define WEGERT_FUNCTION_H

#define WEGERT_MAX_FACTORS 64

struct wegert_function {
    float zeros[WEGERT_MAX_FACTORS][2];
    float poles[WEGERT_MAX_FACTORS][2];
    int zero_count;
    int pole_count;
};

void wegert_function_initialize_default(struct wegert_function *function);
void wegert_function_clear(struct wegert_function *function);

#endif
