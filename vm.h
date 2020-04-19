#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "value.h"
#include "table.h"

#define STACK_MAX 256

struct vm {
	struct chunk *chunk; /* chunk of bytecode*/
	uint8_t *ip; /* instruction pointer */
	value_t stack[STACK_MAX]; /* the virtual machine's stack*/
	value_t *stack_top; /* the top of the vm's stack */
	struct obj *objects; /* head of list of objects to be tracked by vm*/
	struct table strings; /* hash table of interned strings */
};

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,

} interpret_result_t;

extern struct vm vm;
void init_vm();
void free_vm();
interpret_result_t interpret_vm(const char *c);

void push(value_t val);
value_t pop();

#endif