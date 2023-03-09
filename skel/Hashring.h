/* Copyright 2021 Nastase Maria 311CA */
#ifndef __HASHRING_H
#define __HASHRING_H

#include "server.h"
#include "utils.h"

typedef struct hashring_t hashring_t;
struct hashring_t {
	int size;
    int capacity;
    server_memory **servers;
};

hashring_t* init_hashring();

server_memory* init_server_hashring(server_memory *server_lb);

void hr_store(hashring_t* hr, char* key, char* value, int *server_id);

char* hr_retrieve(hashring_t *hr, char* key, int *server_id);

void resize_hashring_double(hashring_t *hr);

int replicate_id(int server_id, int nr_replica);

void add_server_hashring(hashring_t* hr, server_memory *server);

void remove_server_hashring(hashring_t* hr, int server_id, int *server_index_load_balancer);

void free_hashring(hashring_t* hr);

#endif