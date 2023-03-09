/* Copyright 2021 Nastase Maria 311CA */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

#include "Hashring.h"

int binary_search(hashring_t* hr, int hash_search) {
    int left = 0, right = hr->size - 1;
    if (hash_search <= hr->servers[0]->hash_server) {
        return 0;
    }
    while (left < right) {
        int middle = (left + right) / 2;
        if (hash_search > hr->servers[middle]->hash_server) {
            right = middle;
        } else if (hash_search < hr->servers[middle]->hash_server){
            if (hash_search > hr->servers[middle - 1]->hash_server)
                return middle;
            left = middle;
        } else {
            return middle;
        }
    }
}

void redristribute_data(server_memory *store_server, server_memory *server_retrieve)
{
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
    hashring_t *hr = (hashring_t *)malloc(sizeof(hashring_t));
    DIE(hr == NULL, "Hashring Malloc");
    hr->size = 0;
    hr->capacity = 1;
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
    int index_server_hr = binary_search(hr, hash_function_key(key));
    *server_id = hr->servers[index_server_hr]->id_server;
    server_store(hr->servers[index_server_hr], key, value);
}

char* hr_retrieve(hashring_t* hr, char* key, int *server_id)
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
        int id_replica = replicate_id(server_id, i);
        int hash_server = hash_function_servers(&id_replica);
        if (hr->size != 0) {
            index_server_store = binary_search(hr, hash_server); 
        }
        if (hr->servers[index_server_store]->hash_server == hash_server) {
            if (id_replica > hr->servers[index_server_store]->id_server)
                index_server_store++;
        }
        for (int j = index_server_store + 1; j < hr->size; j++) {
            hr->servers[j] = hr->servers[j - 1];
        }
        redristribute_data(hr->servers[index_server_store], hr->servers[index_server_store + 1]);
        hr->servers[index_server_store] = init_server_hashring(server);
        init_info_server(hr->servers[index_server_store], server_id, index_in_load_balancer);
    }
}

void remove_server_hashring(hashring_t* hr, int server_id, int *server_index_load_balancer)
{
    if (hr->size == 0)
         return;
    for (int i = 0; i < 3; i++) {
        int id_replica = replicate_id(server_id, i);
        int hash_server = hash_function_servers(&server_id);
        int server_index = binary_search(hr, hash_server);
        *server_index_load_balancer = hr->servers[server_index]->index_server;
        redristribute_data(hr->servers[server_index + 1], hr->servers[server_index]);
        for (int j = server_index; j < hr->size - 1; ++j)
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
