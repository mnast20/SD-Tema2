/* Copyright 2021 Nastase Maria 311CA */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

#include "Hashtable.h"

#define MAX_BUCKET_SIZE 64

int
compare_function_ints(void *a, void *b)
{
	int int_a = *((int *)a);
	int int_b = *((int *)b);

	if (int_a == int_b) {
		return 0;
	} else if (int_a < int_b) {
		return -1;
	} else {
		return 1;
	}
}

int
compare_function_strings(void *a, void *b)
{
	char *str_a = (char *)a;
	char *str_b = (char *)b;

	return strcmp(str_a, str_b);
}

/*
 * Functii de hashing:
 */
// unsigned int
// hash_function_int(void *a)
// {
// 	/*
// 	 * Credits: https://stackoverflow.com/a/12996028/7883884
// 	 */
// 	unsigned int uint_a = *((unsigned int *)a);

// 	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
// 	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
// 	uint_a = (uint_a >> 16u) ^ uint_a;
// 	return uint_a;
// }

// unsigned int
// hash_function_string(void *a)
// {
// 	/*
// 	 * Credits: http://www.cse.yorku.ca/~oz/hash.html
// 	 */
// 	unsigned char *puchar_a = (unsigned char*) a;
// 	unsigned long hash = 5381;
// 	int c;

// 	while ((c = *puchar_a++))
// 		hash = ((hash << 5u) + hash) + c; /* hash * 33 + c */

// 	return hash;
// }

unsigned int hash_function_key(void *a) {
    unsigned char *puchar_a = (unsigned char *) a;
    unsigned int hash = 5381;
    int c;

    while ((c = *puchar_a++))
        hash = ((hash << 5u) + hash) + c;

    return hash;
}

/*
 * Functie apelata dupa alocarea unui hashtable pentru a-l initializa.
 * Trebuie alocate si initializate si listele inlantuite.
 */
void init_ht(struct Hashtable *ht, unsigned int hmax)
{
	ht->buckets = (struct LinkedList **)malloc(hmax * sizeof(struct LinkedList *));
	for (int i = 0; i < hmax; i++) {
		ht->buckets[i] = ll_create(MAX_BUCKET_SIZE);
	}
	ht->size = 0;
	ht->hmax = hmax;
}

int
key_position(Hashtable *ht, LinkedList *list, void *key)
{
	int nr = 0;
	Node *current = list->head;
	while (current!= NULL) {
		struct info* information = current->data;
		if (compare_function(key, information->key) == 0) {
			return nr;
		}
		current = current->next;
		nr++;
	}
	return -1;
}

struct Node* get_node(struct Hashtable *ht, LinkedList* list, void *key)
{
	Node *current = list->head;
	while (current!= NULL) {
		struct info* information = current->data;
		if (compare_function_strings(key, information->key) == 0) {
			return current;
		}
		current = current->next;
	}
	return NULL;
}

/*
 * Atentie! Desi cheia este trimisa ca un void pointer (deoarece nu se impune tipul ei), in momentul in care
 * se creeaza o noua intrare in hashtable (in cazul in care cheia nu se gaseste deja in ht), trebuie creata o copie
 * a valorii la care pointeaza key si adresa acestei copii trebuie salvata in structura info asociata intrarii din ht.
 * Pentru a sti cati octeti trebuie alocati si copiati, folositi parametrul key_size_bytes.
 *
 * Motivatie:
 * Este nevoie sa copiem valoarea la care pointeaza key deoarece dupa un apel put(ht, key_actual, value_actual),
 * valoarea la care pointeaza key_actual poate fi alterata (de ex: *key_actual++). Daca am folosi direct adresa
 * pointerului key_actual, practic s-ar modifica din afara hashtable-ului cheia unei intrari din hashtable.
 * Nu ne dorim acest lucru, fiindca exista riscul sa ajungem in situatia in care nu mai stim la ce cheie este
 * inregistrata o anumita valoare.
 */
void
put(Hashtable *ht, void *key, unsigned int key_size,
	void *value, unsigned int value_size)
{
	int n;
	int index = hash_function_key(key) % ht->hmax;
	struct info *new = (struct info*)malloc(sizeof(struct info));
	new->key = malloc(key_size);
	new->value = malloc(value_size);
	memcpy(new->key, key, key_size);
	memcpy(new->value, value, value_size);
	LinkedList *list = ht->buckets[index];
	struct Node* node = get_node(ht, list, key);
	if (node == NULL) {
		ll_add_nth_node(list, list->size, new);
		ht->size++;
	}
	else {
		node->data = new;
	}
}

void *
get_value(Hashtable *ht, void *key)
{
	int index = hash_function_key(key) % ht->hmax;
	LinkedList *list = ht->buckets[index];
	Node *current = list->head;
	while (current!= NULL) {
		struct info* information = current->data;
		if (compare_function_strings(key, information->key) == 0) {
			return information->value;
		}
		current = current->next;
	}
	return NULL;
}

/*
 * Functie care intoarce:
 * 1, daca pentru cheia key a fost asociata anterior o valoare in hashtable folosind functia put
 * 0, altfel.
 */
int
has_key(Hashtable *ht, void *key)
{
	if (get_value(ht, key))
		return 1;
	return 0;
}

/*
 * Procedura care elimina din hashtable intrarea asociata cheii key.
 * Atentie! Trebuie avuta grija la eliberarea intregii memorii folosite pentru o intrare din hashtable (adica memoria
 * pentru copia lui key --vezi observatia de la procedura put--, pentru structura info si pentru structura Node din
 * lista inlantuita).
 */
void
remove_ht_entry(Hashtable *ht, void *key)
{
	int index = hash_function_key(key) % ht->hmax;
	int n;
	LinkedList *list = ht->buckets[index];
	Node *current = list->head;
	n = key_position(ht, list, key);
	if (n > 0) {
		ll_remove_nth_node(list, n);
	}
}

/*
 * Procedura care elibereaza memoria folosita de toate intrarile din hashtable, dupa care elibereaza si memoria folosita
 * pentru a stoca structura hashtable.
 */
void free_info(struct info* information)
{
	free(information->key);
	free(information->value);
}

void
free_ht(Hashtable *ht)
{
	struct Node *curr, *tmp;
	for (int i = 0; i < ht->hmax; i++) {
		LinkedList *list = ht->buckets[i];
		curr = list->head;
		while (curr != NULL) {
			tmp = curr->next;
			free_info(curr->data);
			free(curr->data);
			free(curr);
			curr = tmp;
		}
		free(list);
	}
	free(ht->buckets);
	free(ht);
}

unsigned int
ht_get_size(Hashtable *ht)
{
	if (ht == NULL)
		return 0;

	return ht->size;
}

unsigned int
ht_get_hmax(Hashtable *ht)
{
	if (ht == NULL)
		return 0;

	return ht->hmax;
}
