#include"memory.h"
#include"object.h"
#include"vm.h"


void *reallocate(void *p, size_t old_size, size_t new_size)
{
        if(!new_size) {
                free(p);
                return NULL;
        }
        return realloc(p, new_size);
}

static void free_object(struct obj *obj)
{
        switch(obj->type) {
                case OBJ_STRING: {
                        obj_string_t *s = (obj_string_t*)obj;
                        FREE_ARRAY(char, s->chars, s->length + 1);
                        FREE(obj_string_t, obj);
                        break;
                }
        }
}

void free_objects()
{
        struct obj *obj = vm.objects;
        while(obj) {
                struct obj *nxt = obj->next;
                free_object(obj);
                obj = nxt;
        }
}