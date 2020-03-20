#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include<string.h>

#include"common.h"


#define GROW_CAPACITY(c) ((c) < 8 ? 8 : (c) * 2)
#define GROW_ARRAY(array, type, old_cnt, cnt) \
        (type *)reallocate((array), sizeof(type) * old_cnt, sizeof(type) * cnt)

#define FREE_ARRAY(type, array, old_cnt) \
        (type *)reallocate((array), sizeof(type) * old_cnt, 0)

#define ZERO_INITIALIZE(array, type, old_l, new_l) \
        memset((array), 0, ((new_l) - (old_l)) * sizeof(type)) 
/*
* All memory allocations/deallocations should be routed through reallocate.
* This will make it easier to track memory in our garbage collector. To 
* deallocate, just pass in 0 for new_size.
*/
void *reallocate(void *p, size_t old_size, size_t new_size);

#endif
