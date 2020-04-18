
#include<stdio.h>
#include<string.h>

#include"object.h"
#include"memory.h"
#include"value.h"
#include"vm.h"


#define ALLOCATE_OBJ(type, obj_type)    (type*)allocate_object(sizeof(type), obj_type)

void print_object(value_t val)
{
        switch(OBJ_TYPE(val)) {
                case OBJ_STRING:
                        printf("%s", AS_CSTRING(val));
                        break;

        }
}

static struct obj *allocate_object(size_t size, obj_type_t type)
{
        struct obj *obj = (struct obj*)reallocate(NULL, 0, size);
        obj->type = type;

        obj->next = vm.objects; /* insert in front of list*/
        vm.objects = obj;

        return obj;
}



static obj_string_t *allocate_string(char *chars, int length)
{
        obj_string_t *string = ALLOCATE_OBJ(obj_string_t, OBJ_STRING);
        string->length = length;
        string->chars = chars;

        return string;
}

obj_string_t *take_string(char *chars, int len)
{
        return allocate_string(chars, len);
}

obj_string_t *copy_string(char *chars, int length)
{
        char *heap_chars = ALLOCATE(char, length + 1);        
        memcpy(heap_chars, chars, length);
        heap_chars[length] = '\0';
        return allocate_string(heap_chars, length);
}