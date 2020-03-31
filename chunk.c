
#include<stdlib.h>
#include"chunk.h"
#include"memory.h"



void init_chunk(struct chunk *c)
{
        c->count = 0;
        c->capacity = 0;
        c->code = NULL;
        c->lines = NULL;
        c->curr_line = 0;
        c->lines_capacity = 0;
        init_value_array(&c->constants);
}

void free_chunk(struct chunk *c)
{
        FREE_ARRAY(uint8_t, c->code, c->capacity);
        FREE_ARRAY(int, c->lines, c->lines_capacity);
        free_value_array(&c->constants);
        init_chunk(c);
}

void write_chunk(struct chunk *c, uint8_t byte, int line)
{
        if(c->count + 1 > c->capacity) {
                int old = c->capacity;
                c->capacity = GROW_CAPACITY(old);
                c->code = GROW_ARRAY(c->code, uint8_t, old, c->capacity);
        }

        if(c->curr_line + 2 >= c->lines_capacity) {
                int old = c->lines_capacity;
                c->lines_capacity = GROW_CAPACITY(old);
                c->lines = GROW_ARRAY(c->lines, int, old, c->lines_capacity);
                ZERO_INITIALIZE(c->lines + old, int, old, c->lines_capacity);
        } 

        c->code[c->count] = byte;
        c->count++;

        if(line == c->lines[c->curr_line]) {
                c->lines[c->curr_line + 1]++;
                
        } else {
                c->curr_line += 2;
                c->lines[c->curr_line] = line;
                c->lines[c->curr_line + 1]++;

        }     
}


int add_constant(struct chunk *c, value_t val)
{
        write_value_array(&c->constants, val);
        return c->constants.count - 1;
}


int write_constant(struct chunk *c, value_t val, int line)
{
        write_chunk(c, OP_CONSTANT_LONG, line);
        int const_idx = add_constant(c, val);
        /* write the index of the constant as a 24 bit integer*/
        uint8_t idx1 = (const_idx >> 16) & 0xff;
        uint8_t idx2 = (const_idx >> 8) & 0xff;
        uint8_t idx3 =  const_idx & 0xff;

        write_chunk(c, idx1, line);
        write_chunk(c, idx2, line);
        write_chunk(c, idx3, line);

        return const_idx; 
}

int get_line_number(struct chunk *c, int offset)
{       
        if(!c || !c->lines) 
                return -1;

        int i = 0;
        int tally = c->lines[1]; 

        while(offset + 1 > tally) {
                i += 2;
                tally += c->lines[i - 1];
        }
        return c->lines[i - 2]; 
}