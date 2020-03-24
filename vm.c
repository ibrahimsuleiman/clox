#include<stdio.h>
#include"vm.h"
#include"common.h"
#include"debug.h"
#include"compiler.h"

struct vm vm;

static inline void reset_stack()
{
        vm.stack_top = vm.stack;
}

static interpret_result_t run_vm(uint8_t *code)
{       

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG(i) (vm.chunk->constants.values[i])
#define OP_BINARY(o)                    \
        do{                             \
                double b = pop();       \
                double a  = pop();      \
                push(a o b);            \
        } while(0)                      \

        uint8_t instruction;
        for(;;) {

#ifdef DEBUG_TRACE_EXECUTION
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
        printf("          ");
        for(value_t *s = vm.stack; s != vm.stack_top; ++s) {
                printf("[");
                print_value(*s);
                printf(" ]");
        }
        printf("\n");
#endif
                switch(instruction = READ_BYTE()) {

                case OP_RETURN:
                        print_value(pop());
                        printf("\n");
                        return INTERPRET_OK;
                case OP_CONSTANT: {
                        value_t constant = READ_CONSTANT();
                        push(constant);
                        break;
                        }
                case OP_CONSTANT_LONG: {
                        uint32_t i = READ_BYTE();
                        i = (i << 8) | READ_BYTE();
                        i = (i << 8) | READ_BYTE();

                        value_t constant = READ_CONSTANT_LONG(i);
                        push(constant);
                        break;                        

                        }
                case OP_NEGATE:
                        push(-pop());
                        break;
                case OP_ADD:
                        OP_BINARY(+); break;
                case OP_SUB:
                        OP_BINARY(-); break;
                case OP_MULT:
                        OP_BINARY(*); break;
                case OP_DIV:
                        OP_BINARY(/); break;
                }
        }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP

}

void init_vm()
{
        vm.chunk = NULL;
        vm.ip = NULL;
        reset_stack();
}

void free_vm()
{

}

interpret_result_t interpret_vm(const char *src)
{
        compile(src);
        return INTERPRET_OK;
}

/* Error checking? */
void push(value_t val)
{
        assert(1 + (int)(vm.stack_top - vm.stack) <= STACK_MAX);
        *vm.stack_top = val;
        vm.stack_top++;
}

value_t pop()
{       
        assert(vm.stack_top - vm.stack >= 0);
        vm.stack_top--;
        return *vm.stack_top;
}