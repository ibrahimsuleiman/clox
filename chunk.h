#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

typedef enum {
	OP_RETURN,
	OP_CONSTANT,
	OP_NIL,
	OP_FALSE,
	OP_TRUE,
	OP_CONSTANT_LONG,
	OP_NEGATE,
	OP_ADD,
	OP_SUB,
	OP_MULT,
	OP_DIV,
	OP_NOT,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
	OP_PRINT,
	OP_POP,
	OP_DEFINE_GLOBAL,
	OP_GET_GLOBAL,
	OP_SET_GLOBAL,

} op_code;

struct chunk {
	int count;
	int capacity;
	uint8_t *code; /* bytes of instructions*/

	struct value_array constants; /* constant data pool*/
	int *lines; /*line number to report errors*/
	int curr_line; /* the line_no of the current instruction*/
	int lines_capacity; /* capacity of the lines array */
};

void init_chunk(struct chunk *c);
void free_chunk(struct chunk *c);
void write_chunk(struct chunk *c, uint8_t byte, int line);
void undo_last_write_to_chunk(struct chunk *c);
/* returns the index where val was added*/
int add_constant(struct chunk *c, value_t val);
int write_constant(struct chunk *c, value_t val, int line);
/*given the offset of an instruction, return it's line number*/
int get_line_number(struct chunk *c, int offset);

#endif
