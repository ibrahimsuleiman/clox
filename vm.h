#ifndef CLOX_VM_H
#define CLOX_VM_H

#include"chunk.h"
#include"value.h"

#define STACK_MAX 256

struct vm {
        struct chunk *chunk;            /* chunk of bytecode*/
        uint8_t *ip;                    /* instruction pointer */
        value_t stack[STACK_MAX];       /* the virtual machine's stack*/
        value_t *stack_top;             /* the top of the vm's stack */
};

typedef enum {
        INTERPRET_OK,
        INTERPRETE_COMPILE_ERROR,
        INTERPRETE_RUNTIME_ERROR,

}interpret_result_t;

void init_vm(struct vm *vm);
void free_vm(struct vm *vm);
interpret_result_t interpret_vm(struct vm *vm, struct chunk *c);

void push(struct vm *vm, value_t val);
value_t pop(struct vm *vm);


#endif