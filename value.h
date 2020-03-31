#ifndef CLOX_VALUE
#define CLOX_VALUE

#include"common.h"

struct chunk;

typedef double value_t;

struct value_array {
        int count;
        int capacity;
        value_t *values;
};

void init_value_array(struct value_array *v);
void free_value_array(struct value_array *v);
void write_value_array(struct value_array *v, value_t val);
void undo_previous_write(struct value_array *v);
void print_value(value_t val);

#endif