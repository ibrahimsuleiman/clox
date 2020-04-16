#ifndef CLOX_VALUE
#define CLOX_VALUE

#include"common.h"

struct chunk;


typedef enum {
      VAL_BOOL,
      VAL_NIL,
      VAL_NUMBER,  

}value_type_t;

struct value_t {
        value_type_t type;
        union{
                double number;
                bool boolean;
        }as;
};

#define IS_BOOL(value)          ((value.type) == VAL_BOOL)
#define IS_NUMBER(value)        ((value.type) == VAL_NUMBER)
#define IS_NIL(value)           ((value.type) == VAL_NIL)

#define AS_BOOL(value)          ((value).as.boolean)
#define AS_NUMBER(value)        ((value).as.number)

#define BOOL(value)     ((struct value_t){VAL_BOOL, {.boolean = value} })
#define NIL_VAL         ((struct value_t){VAL_NIL, {.number = 0} })
#define NUMBER(value)   ((struct value_t){ VAL_NUMBER, {.number = value} })


typedef struct value_t value_t;

struct value_array {
        int count;
        int capacity;
        value_t *values;
};

void init_value_array(struct value_array *v);
void free_value_array(struct value_array *v);
void write_value_array(struct value_array *v, value_t val);
void undo_previous_write(struct value_array *v);
void print_value(value_t val);

#endif