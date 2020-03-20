#include"common.h"
#include"chunk.h"
#include"debug.h"

int main(int argc, char *argv[])
{
        struct chunk chunk;
        init_chunk(&chunk);
        write_chunk(&chunk, OP_RETURN, 111);

        int idx = add_constant(&chunk, 1.33);
        write_chunk(&chunk, OP_CONSTANT, 111);
        write_chunk(&chunk, idx, 111); /*write the index of the constant*/
        write_chunk(&chunk, OP_RETURN, 112);

        write_constant(&chunk, 1.3456, 102);
        write_chunk(&chunk, OP_RETURN, 130);

        disassemble_chunk(&chunk, "test chunk");

        free_chunk(&chunk);

        return 0;
}