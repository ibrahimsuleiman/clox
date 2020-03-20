#include"debug.h"
#include"value.h"

static int simple_instruction(const char *name, int offset)
{
        printf("%s \n", name);
        return offset + 1;
}

static int constant_instruction(
        const char *name, struct chunk *c, int offset)
{
        int const_idx = c->code[offset + 1];
        printf("%-16s %4d '", name, const_idx);
        print_value(c->constants.values[const_idx]);
        printf("'\n");
        return offset + 2;
}

static int long_constant_instruction(
        const char *name, struct chunk *c, int offset)
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
        
        for(int offset = 0; offset < c->count;)
                offset = disassemble_instruction(c, offset);
}

int disassemble_instruction(struct chunk *c, int offset)
{
        printf("%04d ", offset);
        uint8_t instr = c->code[offset];

        if(offset > 0 && 
                get_line_number(c, offset) == get_line_number(c, offset - 1)) {
                printf("   | ");
        } else {
                printf("%4d ", get_line_number(c, offset));
        }
        
        switch(instr) {
                case OP_RETURN:
                        return simple_instruction("OP_RETURN", offset);
                case OP_CONSTANT:
                        return constant_instruction("OP_CONSTANT", c, offset);\
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
                default:
                        printf("Unknown opcode %d\n", instr);
                        return offset + 1;
        }
        
}