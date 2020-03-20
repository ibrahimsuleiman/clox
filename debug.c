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
                        return constant_instruction("OP_CONSTANT", c, offset);
                default:
                        printf("Unknown opcode %d\n", instr);
                        return offset + 1;
        }
        
}