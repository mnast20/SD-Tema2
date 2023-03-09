#ifndef __LINKED_LIST_H_
#define __LINKED_LIST_H_

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

LinkedList*
ll_create(unsigned int data_size);

void
ll_add_nth_node(LinkedList* list, unsigned int n, const void* data);

Node*
ll_remove_nth_node(LinkedList* list, unsigned int n);

unsigned int
ll_get_size(LinkedList* list);

void
ll_free(LinkedList** pp_list);

void
ll_print_int(LinkedList* list);

void
ll_print_string(LinkedList* list);

#endif /* __LINKED_LIST_H_ */
