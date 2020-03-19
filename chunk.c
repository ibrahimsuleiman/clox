
#include<stdlib.h>

#include"chunk.h"
#include"memory.h"

void init_chunk(struct chunk *c)
{
        c->count = 0;
        c->capacity = 0;
        c->code = NULL;
        c->lines = NULL;
        init_value_array(&c->constants);
}

void free_chunk(struct chunk *c)
{
        FREE_ARRAY(uint8_t, c->code, c->capacity);
        FREE_ARRAY(int, c->lines, c->capacity);
        free_value_array(&c->constants);
        init_chunk(c);
}

void write_chunk(struct chunk *c, uint8_t byte, int line)
{
        if(c->count + 1 > c->capacity) {
                int old = c->capacity;
                c->capacity = GROW_CAPACITY(old);
                c->code = GROW_ARRAY(c->code, uint8_t, old, c->capacity);
                c->lines = GROW_ARRAY(c->lines, int, old, c->capacity);
        }

        c->code[c->count] = byte;
        c->lines[c->count] = line;
        c->count++;
}

int add_constant(struct chunk *c, value_t val)
{
        write_value_array(&c->constants, val);
        return c->constants.count - 1;
}