#ifndef KERNEL_LIST_H
#define KERNEL_LIST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Intrusive list node. Embed this inside your structures.
typedef struct list_node {
    struct list_node* prev;
    struct list_node* next;
} list_node_t;

#define LIST_MAGIC 0x11557799

// The intrusive doubly-linked list.
typedef struct {
    uint32_t magic;
    list_node_t* head;
    list_node_t* tail;
    uint32_t size;
} list_t;

// Initialize a list
void list_init(list_t* list);

// Initialize a list node
void list_node_init(list_node_t* node);

// Push a node to the back (tail) of the list
void list_insert_tail(list_t* list, list_node_t* node);

// Pop a node from the front (head) of the list
list_node_t* list_remove_head(list_t* list);

// Remove a specific node from the list
void list_remove(list_t* list, list_node_t* node);

// Check if list contains node
bool list_contains(list_t* list, list_node_t* node);

// Check if list is empty
bool list_is_empty(list_t* list);

// Helper macro to get the containing structure from a list node pointer.
// Example: Task* t = LIST_ENTRY(node, Task, queue_node);
#define LIST_ENTRY(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#endif // KERNEL_LIST_H
