#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

#define NR_MAX 100000
#define REQUEST_LENGTH 1024
#define KEY_LENGTH 128
#define VALUE_LENGTH 65536
#define MAX_BUCKETS 5000

typedef struct Node Node;
struct Node {
    void* data;
    Node* next;
};

typedef struct LinkedList LinkedList;
struct LinkedList {
    Node* head;
    unsigned int data_size;
    unsigned int size;
};

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
    int stored_data;
};

typedef struct server_memory server_memory;
struct server_memory {
	//LinkedList *memory;
	Hashtable *memory;
	unsigned int hash_server;
	int id_server;
	int index_server;
};

typedef struct hashring_t hashring_t;
struct hashring_t {
	int size;
    int capacity;
    server_memory **servers;
};

typedef struct load_balancer load_balancer;
struct load_balancer {
	server_memory **servers;
    int size;
    int capacity;
    hashring_t* hr;
};

LinkedList *
ll_create(unsigned int data_size)
{
    LinkedList* ll;

    ll = malloc(sizeof(*ll));
    DIE(ll == NULL, "linked_list malloc");

    ll->head = NULL;
    ll->data_size = data_size;
    ll->size = 0;

    return ll;
}

/*
 * Pe baza datelor trimise prin pointerul new_data, se creeaza un nou nod care e
 * adaugat pe pozitia n a listei reprezentata de pointerul list. Pozitiile din
 * lista sunt indexate incepand cu 0 (i.e. primul nod din lista se afla pe
 * pozitia n=0). Daca n >= nr_noduri, noul nod se adauga la finalul listei. Daca
 * n < 0, eroare.
 */
void
ll_add_nth_node(LinkedList* list, unsigned int n, const void* new_data)
{
    Node *prev, *curr;
    Node* new_node;

    if (list == NULL) {
        return;
    }

    /* n >= list->size inseamna adaugarea unui nou nod la finalul listei. */
    if (n > list->size) {
        n = list->size;
    } else if (n < 0) {
        return;
    }

    curr = list->head;
    prev = NULL;
    while (n > 0) {
        prev = curr;
        curr = curr->next;
        --n;
    }

    new_node = malloc(sizeof(*new_node));
    DIE(new_node == NULL, "new_node malloc");
    new_node->data = malloc(list->data_size);
    DIE(new_node->data == NULL, "new_node->data malloc");
    memcpy(new_node->data, new_data, list->data_size);

    new_node->next = curr;
    if (prev == NULL) {
        /* Adica n == 0. */
        list->head = new_node;
    } else {
        prev->next = new_node;
    }

    list->size++;
}

/*
 * Elimina nodul de pe pozitia n din lista al carei pointer este trimis ca
 * parametru. Pozitiile din lista se indexeaza de la 0 (i.e. primul nod din
 * lista se afla pe pozitia n=0). Daca n >= nr_noduri - 1, se elimina nodul de
 * la finalul listei. Daca n < 0, eroare. Functia intoarce un pointer spre acest
 * nod proaspat eliminat din lista. Este responsabilitatea apelantului sa
 * elibereze memoria acestui nod.
 */
Node *
ll_remove_nth_node(LinkedList* list, unsigned int n)
{
    Node *prev, *curr;

    if (list == NULL) {
        return NULL;
    }

    if (list->head == NULL) { /* Lista este goala. */
        return NULL;
    }

    /* n >= list->size - 1 inseamna eliminarea nodului de la finalul listei. */
    if (n > list->size - 1) {
        n = list->size - 1;
    }

    curr = list->head;
    prev = NULL;
    while (n > 0) {
        prev = curr;
        curr = curr->next;
        --n;
    }

    if (prev == NULL) {
        /* Adica n == 0. */
        list->head = curr->next;
    } else {
        prev->next = curr->next;
    }

    list->size--;

    return curr;
}

/*
 * Functia intoarce numarul de noduri din lista al carei pointer este trimis ca
 * parametru.
 */
unsigned int
ll_get_size(LinkedList* list)
{
    if (list == NULL) {
        return -1;
    }

    return list->size;
}

/*
 * Procedura elibereaza memoria folosita de toate nodurile din lista, iar la
 * sfarsit, elibereaza memoria folosita de structura lista si actualizeaza la
 * NULL valoarea pointerului la care pointeaza argumentul (argumentul este un
 * pointer la un pointer).
 */
void
ll_free(LinkedList** pp_list)
{
    Node* currNode;

    if (pp_list == NULL || *pp_list == NULL) {
        return;
    }

    while (ll_get_size(*pp_list) > 0) {
        currNode = ll_remove_nth_node(*pp_list, 0);
        free(currNode->data);
        free(currNode);
    }

    free(*pp_list);
    *pp_list = NULL;
}

/*
 * Atentie! Aceasta functie poate fi apelata doar pe liste ale caror noduri STIM
 * ca stocheaza int-uri. Functia afiseaza toate valorile int stocate in nodurile
 * din lista inlantuita separate printr-un spatiu.
 */
void
ll_print_int(LinkedList* list)
{
    Node* curr;

    if (list == NULL) {
        return;
    }

    curr = list->head;
    while (curr != NULL) {
        printf("%d ", *((int*)curr->data));
        curr = curr->next;
    }

    printf("\n");
}

/*
 * Atentie! Aceasta functie poate fi apelata doar pe liste ale caror noduri STIM
 * ca stocheaza string-uri. Functia afiseaza toate string-urile stocate in
 * nodurile din lista inlantuita, separate printr-un spatiu.
 */
void
ll_print_string(LinkedList* list)
{
    Node* curr;

    if (list == NULL) {
        return;
    }

    curr = list->head;
    while (curr != NULL) {
        printf("%s ", (char*)curr->data);
        curr = curr->next;
    }

    printf("\n");
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
Hashtable* init_ht(unsigned int hmax)
{
    Hashtable *ht = malloc(sizeof(Hashtable));
	ht->buckets = (struct LinkedList **)malloc(hmax * sizeof(struct LinkedList *));
	for (int i = 0; i < hmax; i++) {
		ht->buckets[i] = ll_create(sizeof(info));
	}
	ht->size = 0;
	ht->hmax = hmax;
    ht->stored_data = 0;
    return ht;
}

int
key_position(Hashtable *ht, LinkedList *list, void *key)
{
	int nr = 0;
	Node *current = list->head;
	while (current!= NULL) {
		struct info* information = current->data;
		if (compare_function_strings(key, information->key) == 0) {
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
    ht->stored_data = 1;
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

unsigned int hash_function_servers(void *a) {
    unsigned int uint_a = *((unsigned int *)a);

    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = (uint_a >> 16u) ^ uint_a;
    return uint_a;
}

server_memory* init_server_memory() {
	server_memory *server = malloc(sizeof(server_memory));
	DIE(server == NULL, "Server Malloc");
	server->memory = init_ht(MAX_BUCKETS);
	return server;
}

void init_info_server(server_memory* server, int server_id, int index_in_load_balancer, unsigned int hash_server)
{
    server->hash_server = hash_server;
    server->id_server = server_id;
    server->index_server = index_in_load_balancer;
}

void server_store(server_memory* server, char* key, char* value)
{
	put(server->memory, key, strlen(key) + 1, value, strlen(value) + 1);
}

void server_remove(server_memory* server, char* key)
{
	remove_ht_entry(server->memory, key);
}

char* server_retrieve(server_memory* server, char* key)
{
	char* value = get_value(server->memory, key);
	return value;
}

void free_server_memory(server_memory* server)
{
	free_ht(server->memory);
	free(server);
}

int binary_search(hashring_t* hr, unsigned int hash_search) 
{
    int left = 0, right = hr->size - 1;
    if (hash_search <= hr->servers[left]->hash_server) {
        return left;
    } else if (hash_search > hr->servers[right]->hash_server) {
        return hr->size;
    } else if (hash_search == hr->servers[right]->hash_server) {
        return right;
    }
    while (left < right) {
        int middle = (left + right) / 2;
        if (hash_search > hr->servers[middle]->hash_server) {
            if (hash_search < hr->servers[middle + 1]->hash_server)
                return middle + 1;
            left = middle;
        } else if (hash_search < hr->servers[middle]->hash_server){
            if (hash_search > hr->servers[middle - 1]->hash_server)
                return middle;
            right = middle;
        } else {
            return middle;
        }
    }
}

void redristribute_data(server_memory *store_server, server_memory *server_retrieve)
{
    if (server_retrieve->memory->stored_data == 0) {
        return;
    }
    for (int i = 0; i < server_retrieve->memory->size; ++i) {
        for (int j = 0; j < server_retrieve->memory->size; ++j) {
            LinkedList *list = server_retrieve->memory->buckets[i];
            Node *current_node = list->head;
            for (int k = 0; k < list->size; ++k) {
                info *data =current_node->data;
                if (hash_function_key(data->key) < store_server->hash_server) {
                    server_store(store_server, data->key, data->value);
                    server_remove(server_retrieve, data->key);
                }
            }
        }
    }
}

hashring_t* init_hashring() {
    hashring_t *hr = (hashring_t *)malloc(3 * sizeof(hashring_t));
    DIE(hr == NULL, "Hashring Malloc");
    hr->size = 0;
    hr->capacity = 3;
    hr->servers = (server_memory **)malloc(hr->capacity * sizeof(server_memory *));
    DIE(hr->servers == NULL, "Hashring Servers Malloc");
    return hr;
}

void resize_hashring_double(hashring_t *hr) {
    hr->capacity *= 2;
    server_memory **tmp = realloc(hr->servers, hr->capacity * sizeof(server_memory *));
    hr->servers = tmp;
}

int replicate_id(int server_id, int nr_replica)
{
    return nr_replica * 100000 + server_id;
}

server_memory* init_server_hashring(server_memory *server_lb) {
    server_memory *server_hr = malloc(sizeof(server_memory));
	DIE(server_hr == NULL, "Server Malloc");
    server_hr->memory = server_lb->memory;
	return server_hr;
}

// void add_server_hashring(hashring_t* hr, int server_id, int index_in_load_balancer)
// {
//     int index_server_store;
//     for (int i = 0; i < 3; i++) {
//         if (hr->size == hr->capacity)
//             resize_hashring_double(hr);
//         int id_replica = replicate_id(server_id, i);
//         int hash_server = hash_function_servers(&server_id);
//         index_server_store = binary_search(hr, hash_server);
//         if (hr->size == 0) {
//             index_server_store = 0;
//         } else {
//             index_server_store = binary_search(hr, hash_server); 
//         }
//         if (hr->servers[index_server_store]->hash_server == hash_server) {
//             if (id_replica > hr->servers[index_server_store]->id_server)
//                 index_server_store++;
//         }
//         for (int j = index_server_store + 1; j < hr->size; j++) {
//             hr->servers[j] = hr->servers[j - 1];
//         }
//         redristribute_data(hr->servers[index_server_store], hr->servers[index_server_store + 1]);
//         hr->servers[index_server_store] = init_server_memory();
//         init_info_server(hr->servers[index_server_store], server_id, index_in_load_balancer);
//     }
// }

void hr_store(hashring_t* hr, char* key, char* value, int *server_id)
{
    unsigned int hash_key = hash_function_key(key);
    int index_server_hr = binary_search(hr, hash_key);
    if (index_server_hr == hr->size)
        index_server_hr = 0;
    *server_id = hr->servers[index_server_hr]->id_server;
    server_store(hr->servers[index_server_hr], key, value);
}

char* hr_retrieve(hashring_t *hr, char* key, int *server_id)
{
    int index_server_hr = binary_search(hr, hash_function_key(key));
    *server_id = hr->servers[index_server_hr]->id_server;
    return server_retrieve(hr->servers[index_server_hr], key);
}

void add_server_hashring(hashring_t* hr, server_memory *server)
{
    int server_id = server->id_server;
    int index_in_load_balancer = server->index_server;
    int index_server_store = 0;
    if (hr->size == hr->capacity)
            resize_hashring_double(hr);
    for (int i = 0; i < 3; i++) {
        hr->servers[hr->size] = init_server_hashring(server);
        int id_replica = replicate_id(server_id, i);
        unsigned int hash_server = hash_function_servers(&id_replica);
        if (hr->size != 0) {
            index_server_store = binary_search(hr, hash_server);
            if (index_server_store < hr->size) {
                if (hr->servers[index_server_store]->hash_server == hash_server) {
                    if (id_replica > hr->servers[index_server_store]->id_server)
                        index_server_store++;
                }
            }
        }
        
        for (int j = hr->size; j >= index_server_store + 1; j--) {
            *hr->servers[j] = *hr->servers[j - 1];
            //hr->(servers+j) = hr->(servers+j - 1);
            //memcpy(hr->servers[j], hr->servers[j - 1], sizeof(server_memory));
        }
        
        /*for (int j = hr->size; j >= index_server_store + 1; j--) {
            // hr->servers[j]->hash_server = hr->servers[j - 1]->hash_server;
            server_memory *tmp = hr->servers[j - 1];
            free(hr->servers[j - 1]);
            hr->servers[j] = tmp;
            hr->servers[j - 1] = init_server_hashring(server);
        }*/
        if (hr->size >= 3) {
            if (index_server_store == hr->size) {
                redristribute_data(hr->servers[index_server_store], hr->servers[0]);
            } else {
                redristribute_data(hr->servers[index_server_store], hr->servers[index_server_store + 1]);
            }
        }
        init_info_server(hr->servers[index_server_store], server_id, index_in_load_balancer, hash_server);
        hr->size++;
    }
}

void remove_server_hashring(hashring_t* hr, int server_id, int *server_index_load_balancer)
{
    if (hr->size == 0)
         return;
    for (int i = 0; i < 3; i++) {
        int id_replica = replicate_id(server_id, i);
        unsigned int hash_server = hash_function_servers(&server_id);
        int index_server_remove = binary_search(hr, hash_server);
        *server_index_load_balancer = hr->servers[index_server_remove]->index_server;
        if (index_server_remove == hr->size) {
            redristribute_data(hr->servers[0], hr->servers[index_server_remove]);
        } else {
            redristribute_data(hr->servers[index_server_remove + 1], hr->servers[index_server_remove]);
        }
        for (int j = index_server_remove; j < hr->size - 1; ++j)
            hr->servers[j] = hr->servers[j+1];
    }
    hr->size-=3;
    //return server_index_load_balancer;
}

void free_hashring(hashring_t* hr) {
    for (int i = 0; i < hr->capacity; ++i) {
        free(hr->servers[i]->memory);
        free(hr->servers[i]);
    }
    free(hr);
}

load_balancer* init_load_balancer() {
	load_balancer *main = malloc(sizeof(load_balancer));
    DIE(main == NULL, "Load balancer Malloc");
    main->size = 0;
    main->capacity = 1;
    main->servers = (server_memory **)malloc(main->capacity * sizeof(server_memory *));
    DIE(main->servers == NULL, "Load balancer Servers Malloc");
    main->hr = init_hashring();
    return main;
    // return NULL;
}

void resize_load_balancer_double(load_balancer *main) {
    main->capacity *= 2;
    server_memory **tmp = realloc(main->servers, main->capacity * sizeof(server_memory *));
    main->servers = tmp;
}

void loader_store(load_balancer* main, char* key, char* value, int* server_id) {
	/* TODO. */
    // int index_server = binary_search(main, hash_function_key(key));
    hr_store(main->hr, key, value, server_id);
}


char* loader_retrieve(load_balancer* main, char* key, int* server_id) {
	/* TODO. */
    int index_server;
    char *value = hr_retrieve(main->hr, key, server_id);
	// return NULL;
}

void loader_add_server(load_balancer* main, int server_id) {
	/* TODO. */
    if (main->size == main->capacity) {
        resize_load_balancer_double(main);
    }
    main->servers[main->size] = init_server_memory();
    init_info_server(main->servers[main->size], server_id, main->size, hash_function_servers(&server_id));
    //add_server_hashring(main->hr, server_id, main->size);
    add_server_hashring(main->hr, main->servers[main->size]);
    main->size++;
    
}

void loader_remove_server(load_balancer* main, int server_id) {
	/* TODO. */
    /*for (int i = 0; i < 3; i++) {
        int id_replica = replicate_id(server_id, i);
        int hash_server = hash_function_servers(&server_id);
        int server_index = binary_search(main, hash_server);
        redristribute_data(main->servers[server_index + 1], main->servers[server_index]);
        for (int j = server_index; j < main->size - 1; ++j)
            main->servers[j] = main->servers[j+1];
    }*/
    int server_index = -1;
    remove_server_hashring(main->hr, server_id, &server_index);
    if (server_index != -1) {
        for (int i = server_index; i < main->size - 1; ++i)
            main->servers[i] = main->servers[i+1];
        main->size--;
    }
}

void free_load_balancer(load_balancer* main) {
    for (int i = 0; i < main->capacity; ++i) {
        free_server_memory(main->servers[i]);
    }
    free(main);
}

void get_key_value(char* key, char* value, char* request) {
	int key_start = 0, value_start = 0;
	int key_finish = 0, value_finish = 0;
	int key_index = 0, value_index = 0;

	for (unsigned int i = 0; i < strlen(request); ++i) {
		if (request[i] == '"' && value_start != 1) {
			if (key_start == 0) {
				key_start = 1;
			} else if (key_finish == 0) {
				key_finish = 1;
			} else if (value_start == 0) {
				value_start = 1;
			}
		} else {
			if (key_start == 1 && key_finish == 0) {
				key[key_index++] = request[i];
			} else if (value_start == 1 && value_finish == 0) {
				value[value_index++] = request[i];
			}
		}
	}

	value[value_index - 1] = 0;
}

void get_key(char* key, char* request) {
	int key_start = 0, key_index = 0;

	for (unsigned int i = 0; i < strlen(request); ++i) {
		if (request[i] == '"') {
			key_start = 1;
		} else if (key_start == 1) {
			key[key_index++] = request[i];
		}
	}
}

void apply_requests(FILE* input_file) {
	char request[REQUEST_LENGTH] = {0};
	char key[KEY_LENGTH] = {0};
	char value[VALUE_LENGTH] = {0};
	load_balancer* main_server = init_load_balancer();

	while (fgets(request, REQUEST_LENGTH, input_file)) {
		request[strlen(request) - 1] = 0;
		if (!strncmp(request, "store", sizeof("store") - 1)) {
			get_key_value(key, value, request);

			int index_server = 0;
			loader_store(main_server, key, value, &index_server);
			printf("Stored %s on server %d.\n", value, index_server);

			memset(key, 0, sizeof(key));
			memset(value, 0, sizeof(value));
		} else if (!strncmp(request, "retrieve", sizeof("retrieve") - 1)) {
			get_key(key, request);

			int index_server = 0;
			char *retrieved_value = loader_retrieve(main_server,
											key, &index_server);
			if (retrieved_value) {
				printf("Retrieved %s from server %d.\n",
						retrieved_value, index_server);
			} else {
				printf("Key %s not present.\n", key);
			}

			memset(key, 0, sizeof(key));
		} else if (!strncmp(request, "add_server", sizeof("add_server") - 1)) {
			int server_id = atoi(request + sizeof("add_server"));

			loader_add_server(main_server, server_id);
		} else if (!strncmp(request, "remove_server",
					sizeof("remove_server") - 1)) {
			int server_id = atoi(request + sizeof("remove_server"));

			loader_remove_server(main_server, server_id);
		} else {
			DIE(1, "unknown function call");
		}
	}

	free_load_balancer(main_server);
}

/*
int main(int argc, char* argv[]) {
	FILE *input;

	if (argc != 2) {
		printf("Usage:%s input_file \n", argv[0]);
		return -1;
	}

	input = fopen(argv[1], "rt");
	DIE(input == NULL, "missing input file");

	apply_requests(input);

	fclose(input);

	return 0;
}
*/
int main(void) {
	FILE *input;
    char s[1001];
    scanf("%s", s);

	input = fopen(s, "rt");
	DIE(input == NULL, "missing input file");

	apply_requests(input);

	fclose(input);

	return 0;
}

/*

Tema2_aux/2021-tema2-v3/in/test1.in

*/