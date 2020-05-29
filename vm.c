#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "vm.h"
#include "common.h"
#include "debug.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"

struct vm vm;

static value_t clock_native(int argc, value_t *args)
{
	return NUMBER((double)clock() / CLOCKS_PER_SEC);
}

static inline void reset_stack()
{
	vm.stack_top = vm.stack;
	vm.frame_count = 0;
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

	/* vomit stack trace*/
	for (int i = vm.frame_count - 1; i >= 0; i--) {
		struct call_frame *frame = &vm.frames[i];
		obj_function_t *function = frame->function;
		/* ip points to next instruction*/

		size_t instruction =
			frame->ip - frame->function->chunk.code - 1;
		fprintf(stderr, "[line %d] in ",
			get_line_number(&function->chunk, instruction));

		if (function->name) {
			fprintf(stderr, "%s() \n", function->name->chars);
		} else {
			fprintf(stderr, "script\n");
		}
	}

	reset_stack();
}

static void define_native(const char *name, native_fn_t function)
{
	push(OBJ(copy_string(name, (int)strlen(name))));
	push(OBJ(new_native(function)));
	table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
	pop();
	pop();
}

static bool call(obj_function_t *f, uint8_t argc)
{
	if (f->arity != argc) {
		runtime_error("Expected %d arguments but got %d", f->arity,
			      argc);
		return false;
	}

	if (vm.frame_count == FRAMES_MAX) {
		runtime_error("Stack Overflow.");
		return false;
	}

	struct call_frame *frame = &vm.frames[vm.frame_count++];
	frame->function = f;
	frame->ip = f->chunk.code;
	/* args on stack line up with function parameters.*/
	frame->slots = vm.stack_top - argc - 1;

	return true;
}

static bool call_value(value_t callee, uint8_t argc)
{
	if (IS_OBJ(callee)) {
		switch (OBJ_TYPE(callee)) {
		case OBJ_FUNCTION:
			return call(AS_FUNCTION(callee), argc);
		case OBJ_NATIVE: {
			native_fn_t native = AS_NATIVE(callee);
			value_t result = native(argc, vm.stack_top - argc);
			vm.stack_top -= argc + 1;
			push(result);
			return true;
		}

		default:
			break;
		}
	}

	runtime_error("Attempt to call a non-callable type.");
	return false;
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
	memcpy(chars, b->chars, b->length);
	memcpy(chars + b->length, a->chars, a->length);
	chars[len] = '\0';

	obj_string_t *r = take_string(chars, len);
	push(OBJ(r));
}

static interpret_result_t run_vm()
{
	struct call_frame *frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
	(frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | (frame->ip[-1])))
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG(i) (frame->function->chunk.constants.values[i])
#define READ_STRING() AS_STRING(READ_CONSTANT())

#define OP_BINARY(value_type, o)                                   \
	do {                                                       \
		if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {  \
			runtime_error("Operands must be numbers"); \
			return INTERPRET_RUNTIME_ERROR;            \
		}                                                  \
		double b = AS_NUMBER(pop());                       \
		double a = AS_NUMBER(pop());                       \
		push(value_type(a o b));                           \
	} while (0)

	uint8_t instruction;

	for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
		disassemble_instruction(
			&frame->function->chunk,
			(int)(frame->ip - frame->function->chunk.code));
		printf("          ");
		for (value_t *s = vm.stack; s != vm.stack_top; ++s) {
			printf("[");
			print_value(*s);
			printf(" ]");
		}
		printf("\n");
#endif
		switch (instruction = READ_BYTE()) {
		case OP_RETURN: {
			value_t result = pop();
			vm.frame_count--;
			if (vm.frame_count == 0) {
				pop();
				return INTERPRET_OK;
			}

			vm.stack_top = frame->slots;
			push(result);

			frame = &vm.frames[vm.frame_count - 1];
			break;
		}

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
			if (!IS_NUMBER(peek(0))) {
				runtime_error("Operand must be a number");
				return INTERPRET_RUNTIME_ERROR;
			}
			push(NUMBER(-AS_NUMBER(pop())));
			break;
		case OP_ADD:
			if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
				concatenate();
				break;
			} else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
				double a = AS_NUMBER(pop());
				double b = AS_NUMBER(pop());

				push(NUMBER(a + b));
				break;
			} else {
				runtime_error(
					"Operands must be two numbers or two strings");
				return INTERPRET_RUNTIME_ERROR;
			}
		case OP_SUB:
			OP_BINARY(NUMBER, -);
			break;
		case OP_MULT:
			OP_BINARY(NUMBER, *);
			break;
		case OP_DIV:
			OP_BINARY(NUMBER, /);
			break;
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
			OP_BINARY(BOOL, >);
			break;
		case OP_LESS:
			OP_BINARY(BOOL, <);
			break;
		case OP_PRINT:
			print_value(pop());
			printf("\n");
			break;
		case OP_POP:
			pop();
			break;
		case OP_DEFINE_GLOBAL: {
			obj_string_t *name = READ_STRING();
			table_set(&vm.globals, name, peek(0));
			pop();
			break;
		}
		case OP_GET_GLOBAL: {
			obj_string_t *name = READ_STRING();
			value_t val;
			if (!table_get(&vm.globals, name, &val)) {
				runtime_error("Undefined variable '%s'",
					      name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}
			push(val);
			break;
		}
		case OP_SET_GLOBAL: {
			obj_string_t *name = READ_STRING();
			if (table_set(&vm.globals, name, peek(0))) {
				table_delete(&vm.globals, name); /* delete
								    zombie
								    value*/
				runtime_error("Undefined variable '%s'.",
					      name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}

			break;
		}

		case OP_GET_LOCAL: {
			uint8_t slot = READ_BYTE();
			push(frame->slots[slot]);
			break;
		}

		case OP_SET_LOCAL: {
			/* we don't pop: result of asgnt is the expr on the
			 * left*/
			uint8_t slot = READ_BYTE();
			frame->slots[slot] = peek(0);
			break;
		}

		case OP_JUMP_IF_FALSE: {
			uint16_t offset = READ_SHORT();
			if (is_falsy(peek(0)))
				frame->ip += offset;
			break;
		}
		case OP_JUMP: {
			/* unconditional jump*/
			uint16_t offset = READ_SHORT();
			frame->ip += offset;
			break;
		}
		case OP_LOOP: {
			uint16_t offset = READ_SHORT();
			frame->ip -= offset;
			break;
		}
		case OP_CALL: {
			uint8_t argc = READ_BYTE();
			if (!call_value(peek(argc), argc)) {
				return INTERPRET_RUNTIME_ERROR;
			}
			/* update frame*/
			frame = &vm.frames[vm.frame_count - 1];
			break;
		}
		}
	}

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef READ_STRING
#undef BINARY_OP
}

void init_vm()
{
	vm.objects = NULL;
	init_table(&vm.strings);
	init_table(&vm.globals);
	reset_stack();
	define_native("clock", clock_native);
}

void free_vm()
{
	free_table(&vm.globals);
	free_objects();
	free_table(&vm.strings);
}

interpret_result_t interpret_vm(const char *src)
{
	obj_function_t *function = compile(src);
	if (!function)
		return INTERPRET_COMPILE_ERROR;

	reset_stack(); /* prevent stack from needlessly growing in repl mode*/
	push(OBJ(function));

	/* set up first call frame*/
	call_value(OBJ(function), 0);

	interpret_result_t r = run_vm();

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
