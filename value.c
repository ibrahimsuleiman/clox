#include"value.h"
#include"memory.h"


void init_value_array(struct value_array *v)
{
        v->capacity = 0;
        v->count = 0;
        v->values = NULL;
}

void free_value_array(struct value_array *v)
{
        FREE_ARRAY(value_t, v->values, v->capacity);
        init_value_array(v);
}

void write_value_array(struct value_array *v, value_t val)
{
        if(v->count + 1 > v->capacity) {
                int old = v->capacity;
                v->capacity = GROW_CAPACITY(old);
                v->values = GROW_ARRAY(v->values, value_t, old, v->capacity);
                
        }

        v->values[v->count] = val;
        v->count++;
}

void print_value(value_t val)
{
        printf("%g", val);
}