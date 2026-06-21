#include "kernel/lib/include/list.h"

void list_init(list_t* list) {
    if (!list) return;
    list->magic = LIST_MAGIC;
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void list_node_init(list_node_t* node) {
    if (!node) return;
    node->prev = NULL;
    node->next = NULL;
}

void list_insert_tail(list_t* list, list_node_t* node) {
    if (!list || list->magic != LIST_MAGIC || !node) return;

    // Prevent double enqueue or corrupted node
    if (node->next || node->prev || list->head == node) return;

    if (!list->head) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
    list->size++;
}

list_node_t* list_remove_head(list_t* list) {
    if (!list || list->magic != LIST_MAGIC || !list->head) return NULL;

    list_node_t* node = list->head;
    list->head = node->next;

    if (list->head) {
        list->head->prev = NULL;
    } else {
        list->tail = NULL;
    }

    node->next = NULL;
    node->prev = NULL;
    list->size--;

    return node;
}

void list_remove(list_t* list, list_node_t* node) {
    if (!list || list->magic != LIST_MAGIC || !node || list->size == 0) return;

    if (node->prev) {
        node->prev->next = node->next;
    } else {
        // Node is head
        list->head = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    } else {
        // Node is tail
        list->tail = node->prev;
    }

    node->next = NULL;
    node->prev = NULL;
    list->size--;
}

bool list_contains(list_t* list, list_node_t* node) {
    if (!list || list->magic != LIST_MAGIC || !node) return false;
    
    list_node_t* curr = list->head;
    while (curr) {
        if (curr == node) return true;
        curr = curr->next;
    }
    return false;
}

bool list_is_empty(list_t* list) {
    if (!list || list->magic != LIST_MAGIC) return true;
    return list->size == 0;
}
