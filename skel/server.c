/* Copyright 2021 Nastase Maria 311CA */
#include <stdlib.h>
#include <string.h>

#include "server.h"

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
	init_ht(server->memory, 5000);
	return server;
}

void init_info_server(server_memory* server, int server_id, int index_in_load_balancer)
{
    server->hash_server = hash_function_servers(server_id);
    server->id_server = server_id;
    server->index_server = index_in_load_balancer;
}

void server_store(server_memory* server, char* key, char* value) {
	put(server->memory, key, strlen(key) + 1, value, strlen(value) + 1);
}

void server_remove(server_memory* server, char* key) {
	remove_ht_entry(server->memory, key);
}

char* server_retrieve(server_memory* server, char* key) {
	char* value = get_value(server->memory, key);
	return value;
}

void free_server_memory(server_memory* server) {
	free_ht(server->memory);
	free(server);
}
