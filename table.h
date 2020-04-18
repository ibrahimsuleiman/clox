#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include"common.h"
#include"value.h"


struct entry {
        obj_string_t *key;
        value_t value;
};

struct table {
        int count;
        int capacity; /* number of entries + tombstones */
        struct entry *entries;
};

void init_table(struct table *t);
void free_table(struct table *t);

bool table_set(struct table *t, obj_string_t *key, value_t value);
bool table_get(struct table *t, obj_string_t *key, value_t *value);
bool table_delete(struct table *t, obj_string_t *key);
/* copy from one table to another*/
void table_add_all(struct table *from, struct table *to);
obj_string_t *table_find_string(
        struct table *t, const char *chars, int length, uint32_t hash);
#endif