#include<stdio.h>
#include<string.h>
#include<stdarg.h>
#include"vm.h"
#include"common.h"
#include"debug.h"
#include"compiler.h"
#include"object.h"
#include"memory.h"

struct vm vm;

static inline void reset_stack()
{
        vm.stack_top = vm.stack;
}

static value_t peek(int dist)
{
        return vm.stack_top[-1 - dist];
}

static void runtime_error(const char *format, ...)
{
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fputs("\n", stderr);

        size_t instruction = vm.ip - vm.chunk->code - 1;
        int line = get_line_number(vm.chunk, instruction);
        fprintf(stderr, "[line %d] in script\n", line);

        reset_stack();
}

static bool is_falsy(value_t v)
{
        return (IS_NIL(v) || (IS_BOOL(v) && !(AS_BOOL(v))));
}

static void concatenate()
{
        obj_string_t *a = AS_STRING(pop());
        obj_string_t *b = AS_STRING(pop());

        int len = a->length + b->length;
        char *chars = ALLOCATE(char, len + 1);
        memcpy(chars,b->chars, b->length);
        memcpy(chars + b->length, a->chars, a->length);
        chars[len] = '\0';

        obj_string_t *r = take_string(chars, len);    
        push(OBJ(r));

}

static interpret_result_t run_vm()
{       

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG(i) (vm.chunk->constants.values[i])

#define OP_BINARY(value_type, o)                                        \
        do{                                                             \
                if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {        \
                        runtime_error("Operands must be numbers");      \
                        return INTERPRET_RUNTIME_ERROR;                 \
                }                                                       \
                double b = AS_NUMBER(pop());                            \
                double a  = AS_NUMBER(pop());                           \
                push(value_type(a o b));                                \
        } while(0)                                                      \


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
                        if(!IS_NUMBER(peek(0))) {
                                runtime_error("Operand must be a number");
                                return INTERPRET_RUNTIME_ERROR;
                        }
                        push(NUMBER(-AS_NUMBER(pop())));
                        break;
                case OP_ADD:
                        if(IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                                concatenate();
                                break;
                        } else if(IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                                double a = AS_NUMBER(pop());
                                double b = AS_NUMBER(pop());

                                push(NUMBER(a + b));
                                break;
                        } else {
                                runtime_error("Operands must be two numbers or two strings");
                                return INTERPRET_RUNTIME_ERROR;
                        }
                case OP_SUB:
                        OP_BINARY(NUMBER, -); break;
                case OP_MULT:
                        OP_BINARY(NUMBER, *); break;
                case OP_DIV:
                        OP_BINARY(NUMBER, /); break;
                case OP_FALSE:
                        push(BOOL(false));
                        break;
                case OP_TRUE:
                        push(BOOL(true));
                        break;
                case OP_NIL:
                        push(NIL_VAL);
                        break;
                case OP_NOT:
                        push(BOOL(is_falsy(pop())));
                        break;
                case OP_EQUAL: {
                        value_t a = pop();
                        value_t b = pop();

                        push(BOOL(values_equal(a, b)));
                        break;
                }
                case OP_GREATER:
                        OP_BINARY(BOOL, >); break;
                case OP_LESS:
                        OP_BINARY(BOOL, <); break;
                }
        }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef BINARY_OP

}

void init_vm()
{
        vm.chunk = NULL;
        vm.ip = NULL;
        vm.objects = NULL;
        init_table(&vm.strings);
        reset_stack();
}


void free_vm()
{
        free_objects();
        free_table(&vm.strings);
}

interpret_result_t interpret_vm(const char *src)
{
        struct chunk chunk;
        init_chunk(&chunk);

        if(!compile(src, &chunk)){
                free_chunk(&chunk);
                return INTERPRET_COMPILE_ERROR;
        }

        vm.chunk = &chunk;
        vm.ip = chunk.code;

        interpret_result_t r =  run_vm();

        free_chunk(&chunk);

        return r;
}

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