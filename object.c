
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



static obj_string_t *allocate_string(char *chars, int length, uint32_t hash)
{
        obj_string_t *string = ALLOCATE_OBJ(obj_string_t, OBJ_STRING);
        string->length = length;
        string->chars = chars;
        string->hash = hash;

        table_set(&vm.strings, string, NIL_VAL); /* intern the string*/

        return string;
}

static uint32_t hash_string(const char *key, int len)
{
        uint32_t hash = 2166136261u;
        for (int i = 0; i < len; i++) {                     
                hash ^= key[i];                                      
                hash *= 16777619;                                    
        }                                                      

        return hash; 
}

obj_string_t *take_string(char *chars, int len)
{
        uint32_t hash = hash_string(chars, len);

        obj_string_t *interned = table_find_string(&vm.strings, chars, len, hash);

        if(interned) {
                FREE_ARRAY(char, chars, len + 1);
                return interned;
        }
        return allocate_string(chars, len, hash);
}

obj_string_t *copy_string(char *chars, int length)
{
        uint32_t hash = hash_string(chars, length);

        obj_string_t *interned = table_find_string(&vm.strings, chars, length, hash);
        
        if(interned) return interned;

        char *heap_chars = ALLOCATE(char, length + 1);        
        memcpy(heap_chars, chars, length);
        heap_chars[length] = '\0';

        return allocate_string(heap_chars, length, hash);
}