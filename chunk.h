#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include"common.h"
#include"value.h"

typedef enum {
        OP_RETURN,
        OP_CONSTANT,
} op_code;

struct chunk {
        int     count;
        int     capacity; 
        uint8_t *code;                 /* bytes of instructions*/

        struct value_array constants;   /* constant data pool*/
        int *lines;                     /*line number to report errors*/
};


void init_chunk(struct chunk *c);
void free_chunk(struct chunk *c);
void write_chunk(struct chunk *c, uint8_t byte, int line);
/* returns the index where val was added*/
int add_constant(struct chunk *c, value_t val);


#endif