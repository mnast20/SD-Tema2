/* Copyright 2021 Nastase Maria 311CA */
#ifndef __HASHTABLE_H
#define __HASHTABLE_H

#include "LinkedList.h"
typedef struct info info;
struct info {
	void *key;
	void *value;
};

typedef struct Hashtable Hashtable;
struct Hashtable {
	LinkedList **buckets; /* Array de liste simplu-inlantuite. */
	/* Nr. total de noduri existente curent in toate bucket-urile. */
	unsigned int size;
	unsigned int hmax; /* Nr. de bucket-uri. */
};

void init_ht(struct Hashtable *ht, unsigned int hmax);

int
key_position(Hashtable *ht, LinkedList *list, void *key);

struct Node* get_node(struct Hashtable *ht, LinkedList* list, void *key);

void
put(Hashtable *ht, void *key, unsigned int key_size,
	void *value, unsigned int value_size);

void *
get_value(Hashtable *ht, void *key);

int
has_key(Hashtable *ht, void *key);

void
remove_ht_entry(Hashtable *ht, void *key);

unsigned int
ht_get_size(Hashtable *ht);

unsigned int
ht_get_hmax(Hashtable *ht);

void free_info(struct info* information);

void
free_ht(Hashtable *ht);

/*
 * Functii de comparare a cheilor:
 */
int
compare_function_ints(void *a, void *b);

int
compare_function_strings(void *a, void *b);

/*
 * Functii de hashing:
 */
unsigned int
hash_function_int(void *a);

unsigned int
hash_function_string(void *a);

#endif
