#include"memory.h"


void *reallocate(void *p, size_t old_size, size_t new_size)
{
        if(!new_size) {
                free(p);
                return NULL;
        }
        return realloc(p, new_size);
}