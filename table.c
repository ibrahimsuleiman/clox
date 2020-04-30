#include <string.h>
#include <stdlib.h>

#include "table.h"
#include "value.h"
#include "memory.h"
#include "object.h"

#define TABLE_MAX_LOAD 0.75

void init_table(struct table *t)
{
	t->count = 0;
	t->capacity = 0;
	t->entries = NULL;
}

void free_table(struct table *t)
{
	FREE_ARRAY(struct entry, t->entries, t->capacity);
	init_table(t);
}

static struct entry *find_entry(struct entry *entries, int capacity,
				obj_string_t *key)
{
	uint32_t i = key->hash % capacity;

	struct entry *tombstone = NULL;

	/* probe incase of collisions*/
	for (;;) {
		struct entry *entry = &entries[i];

		if (entry->key == NULL) { /*empty entry*/
			if (IS_NIL(entry->value)) {
				return tombstone != NULL ? tombstone : entry;
			} else {
				/* found tomstone*/
				if (tombstone == NULL)
					tombstone = entry;
			}
		} else if (entry->key == key) {
			return entry; /*key found */
		}

		i = (i + 1) % capacity;
	}
}

static void adjust_capacity(struct table *t, int capacity)
{
	struct entry *entries = ALLOCATE(struct entry, capacity);

	for (int i = 0; i < capacity; ++i) {
		entries[i].key = NULL;
		entries[i].value = NIL_VAL;
	}

	/* "Rehash" into new table */
	t->count = 0; /* clear out count to get rid of tombstones*/
	for (int i = 0; i < t->capacity; ++i) {
		struct entry *entry = &t->entries[i];
		if (!entry->key)
			continue;

		struct entry *dest = find_entry(entries, capacity, entry->key);
		dest->key = entry->key;
		dest->value = entry->value;
		t->count++;
	}

	FREE_ARRAY(struct entry, t->entries, t->capacity);
	t->entries = entries;
	t->capacity = capacity;
}

bool table_set(struct table *t, obj_string_t *key, value_t value)
{
	if (t->count + 1 > t->capacity * TABLE_MAX_LOAD) {
		int capacity = GROW_CAPACITY(t->capacity);
		adjust_capacity(t, capacity);
	}
	struct entry *e = find_entry(t->entries, t->capacity, key);

	bool is_new_key = (e->key == NULL);

	if (is_new_key && IS_NIL(e->value))
		t->count++; /* only increment where there are no tombstones*/

	e->key = key;
	e->value = value;

	return is_new_key;
}

bool table_get(struct table *t, obj_string_t *key, value_t *value)
{
	if (t->count == 0) {
		return false; /* empty? */
	}

	struct entry *e = find_entry(t->entries, t->capacity, key);

	if (!e->key)
		return false;

	*value = e->value;
	return true;
}

void table_add_all(struct table *from, struct table *to)
{
	for (int i = 0; i < from->capacity; ++i) {
		struct entry *e = &from->entries[i];
		if (e) {
			table_set(to, e->key, e->value);
		}
	}
}

obj_string_t *table_find_string(struct table *t, const char *chars, int length,
				uint32_t hash)
{
	if (t->count == 0)
		return NULL;

	uint32_t i = hash % t->capacity;

	for (;;) {
		struct entry *entry = &t->entries[i];
		if (entry->key == NULL) {
			/* empty non-tombstone entry found*/
			if (IS_NIL(entry->value))
				return NULL;
		} else if (entry->key->length == length &&
			   entry->key->hash == hash &&
			   memcmp(entry->key->chars, chars, length) == 0) {
			return entry->key;
		}

		i = (i + 1) % t->capacity;
	}
}

bool table_delete(struct table *t, obj_string_t *key)
{
	if (t->count == 0)
		return false;

	struct entry *entry = find_entry(t->entries, t->capacity, key);

	if (entry->key == NULL)
		return false; /* no such entry*/

	entry->key = NULL;
	entry->value = BOOL(true);

	return true;
}
