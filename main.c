#include"common.h"
#include"chunk.h"
#include"debug.h"
#include"vm.h"

int main(int argc, char *argv[])
{
        
        struct vm clox;
        init_vm(&clox);

        struct chunk chunk;
        init_chunk(&chunk);

        int idx = add_constant(&chunk, 1.33);
        write_chunk(&chunk, OP_CONSTANT, 1);
        write_chunk(&chunk, idx, 2); /*write the index of the constant*/
        write_constant(&chunk, 1.3456, 102);
        write_chunk(&chunk, OP_NEGATE, 3);

        write_chunk(&chunk, OP_DIV, 3);

        write_chunk(&chunk, OP_RETURN, 131);
        

        interpret_vm(&clox, &chunk);

        free_chunk(&chunk);
        free_vm(&clox);

        return 0;
}