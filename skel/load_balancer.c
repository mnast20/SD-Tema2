/* Copyright 2021 Nastase Maria 311CA */
#include <stdlib.h>
#include <string.h>

#include "load_balancer.h"

#define NR_MAX 100000

struct load_balancer {
	server_memory **servers;
    int size;
    int capacity;
    hashring_t *hr;
};

// unsigned int hash_function_servers(void *a) {
//     unsigned int uint_a = *((unsigned int *)a);

//     uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
//     uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
//     uint_a = (uint_a >> 16u) ^ uint_a;
//     return uint_a;
// }

// unsigned int hash_function_key(void *a) {
//     unsigned char *puchar_a = (unsigned char *) a;
//     unsigned int hash = 5381;
//     int c;

//     while ((c = *puchar_a++))
//         hash = ((hash << 5u) + hash) + c;

//     return hash;
// }

// int binary_search(load_balancer* main, int hash_search) {
//     int left = 0, right = main->size - 1;
//     if (hash_search <= main->servers[0]->hash_server) {
//         return 0;
//     }
//     while (left < right) {
//         int middle = (left + right) / 2;
//         if (hash_search > main->servers[middle]->hash_server) {
//             right = middle;
//         } else if (hash_search < main->servers[middle]->hash_server){
//             if (hash_search > main->servers[middle - 1]->hash_server)
//                 return middle;
//             left = middle;
//         } else {
//             return middle;
//         }
//     }
// }

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
    int index_server = binary_search(main, hash_function_key(key));
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
    init_info_server(main->servers[main->size], server_id, main->size);
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
