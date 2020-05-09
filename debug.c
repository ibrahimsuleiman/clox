#include "debug.h"
#include "value.h"

static int simple_instruction(const char *name, int offset)
{
	printf("%s \n", name);
	return offset + 1;
}

static int constant_instruction(const char *name, struct chunk *c, int offset)
{
	int const_idx = c->code[offset + 1];
	printf("%-16s %4d '", name, const_idx);
	print_value(c->constants.values[const_idx]);
	printf("'\n");
	return offset + 2;
}

static int byte_instruction(const char *name, struct chunk *c, int offset)
{
	uint8_t slot = c->code[offset + 1];
	printf("%-16s %4d\n", name, slot);
	return offset + 2;
}

static int long_constant_instruction(const char *name, struct chunk *c,
				     int offset)
{
	uint32_t const_idx = c->code[offset + 1];
	const_idx = (const_idx << 8) | c->code[offset + 2];
	const_idx = (const_idx << 8) | c->code[offset + 3];

	printf("%-16s %4d '", name, const_idx);
	print_value(c->constants.values[const_idx]);
	printf("'\n");
	return offset + 4;
}

void disassemble_chunk(struct chunk *c, const char *name)
{
	printf("== %s ==\n", name);

	for (int offset = 0; offset < c->count;)
		offset = disassemble_instruction(c, offset);
	
	printf("\n");
}

int disassemble_instruction(struct chunk *c, int offset)
{
	printf("%04d ", offset);
	uint8_t instr = c->code[offset];

	if (offset > 0 &&
	    get_line_number(c, offset) == get_line_number(c, offset - 1)) {
		printf("   | ");
	} else {
		printf("%4d ", get_line_number(c, offset));
	}

	switch (instr) {
	case OP_RETURN:
		return simple_instruction("OP_RETURN", offset);
	case OP_CONSTANT:
		return constant_instruction("OP_CONSTANT", c, offset);
	case OP_CONSTANT_LONG:
		return long_constant_instruction("OP_CONSTANT_LONG", c, offset);
	case OP_NEGATE:
		return simple_instruction("OP_NEGATE", offset);
	case OP_ADD:
		return simple_instruction("OP_ADD", offset);
	case OP_SUB:
		return simple_instruction("OP_SUB", offset);
	case OP_MULT:
		return simple_instruction("OP_MULT", offset);
	case OP_DIV:
		return simple_instruction("OP_DIV", offset);
	case OP_FALSE:
		return simple_instruction("OP_FALSE", offset);
	case OP_TRUE:
		return simple_instruction("OP_TRUE", offset);
	case OP_NIL:
		return simple_instruction("OP_NIL", offset);
	case OP_NOT:
		return simple_instruction("OP_NOT", offset);
	case OP_EQUAL:
		return simple_instruction("OP_EQUAL", offset);
	case OP_GREATER:
		return simple_instruction("OP_GREATER", offset);
	case OP_LESS:
		return simple_instruction("OP_LESS", offset);
	case OP_PRINT:
		return simple_instruction("OP_PRINT", offset);
	case OP_POP:
		return simple_instruction("OP_POP", offset);
	case OP_DEFINE_GLOBAL:
		return constant_instruction("OP_DEFINE_GLOBAL", c, offset);
	case OP_GET_GLOBAL:
		return constant_instruction("OP_GET_GLOBAL", c, offset);
	case OP_SET_GLOBAL:
		return constant_instruction("OP_SET_GLOBAL", c, offset);
	case OP_GET_LOCAL:
		return byte_instruction("OP_GET_LOCAL", c, offset);
	case OP_SET_LOCAL:
		return byte_instruction("OP_GET_LOCAL", c, offset);
	default:
		printf("Unknown opcode %d\n", instr);
		return offset + 1;
	}
}
