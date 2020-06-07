#include <string.h>
#include "object.h"
#include "value.h"
#include "memory.h"

bool values_equal(value_t a, value_t b)
{
	if (a.type != b.type)
		return false;
	switch (a.type) {
	case VAL_BOOL:
		return AS_BOOL(a) == AS_BOOL(b);
	case VAL_NUMBER:
		return AS_NUMBER(a) == AS_NUMBER(b);
	case VAL_NIL:
		return true;
	case VAL_OBJECT:
		return AS_OBJ(a) == AS_OBJ(b);
	default:
		/* just here to satisfy the compiler complaining about control
		 * reaching end of non-void fn*/
		return false;
	}
}

void init_value_array(struct value_array *v)
{
	v->capacity = 0;
	v->count = 0;
	v->values = NULL;
}

void free_value_array(struct value_array *v)
{
	FREE_ARRAY(value_t, v->values, v->capacity);
	init_value_array(v);
}

void write_value_array(struct value_array *v, value_t val)
{
	if (v->count + 1 > v->capacity) {
		int old = v->capacity;
		v->capacity = GROW_CAPACITY(old);
		v->values = GROW_ARRAY(v->values, value_t, old, v->capacity);
	}

	v->values[v->count] = val;
	v->count++;
}

void undo_previous_write(struct value_array *v)
{
	v->count--;
}

void print_value(value_t val)
{
	switch (val.type) {
	case VAL_BOOL:
		printf("%s", AS_BOOL(val) ? "true" : "false");
		break;
	case VAL_NUMBER:
		printf("%g", AS_NUMBER(val));
		break;
	case VAL_NIL:
		printf("%s", "nil");
		break;
	case VAL_OBJECT:
		print_object(val);
		break;
	}
}
