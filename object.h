#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "value.h"

typedef enum {
	OBJ_STRING,
} obj_type_t;

/*lox object: all lox objects inherit from this struct*/
struct obj {
	obj_type_t type;
	struct obj *next;
};

struct obj_string {
	struct obj obj; /* base class struct obj*/
	int length; /* string length */
	char *chars; /* character array */
	uint32_t hash; /* cache the hash of string objects for quick lookup*/
};

typedef struct obj_string obj_string_t;

#define OBJ_TYPE(value) (AS_OBJ((value))->type)
#define IS_STRING(value) is_obj_type(value, OBJ_STRING)
#define AS_STRING(value) ((obj_string_t *)AS_OBJ(value))
#define AS_CSTRING(value) (((obj_string_t *)AS_OBJ(value))->chars)

void print_object(value_t val);
obj_string_t *copy_string(char *chars, int length);
obj_string_t *take_string(char *chars, int len);
static inline bool is_obj_type(value_t value, obj_type_t t)
{
	return IS_OBJ(value) && AS_OBJ(value)->type == t;
}

#endif