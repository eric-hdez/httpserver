#include "linkedlist.h"
#include <stdlib.h>

struct list_t {
    node_t *head, *tail;
    size_t size;
};

node_t *node_create(void *data) {
    node_t *n = (node_t *) malloc(sizeof(node_t));
    if (n == NULL) {
        return n;
    }

    n->next = n->prev = NULL;
    n->data = data;
    return n;
}

void node_destroy(node_t **n, void (*del_func_ptr)(void *)) {
    if (n && *n) {
        if (del_func_ptr != NULL && (*n)->data != NULL) {
            del_func_ptr((*n)->data);
        }

        free(*n);
        *n = NULL;
    }
}

list_t *ll_create(void) {
    list_t *ll = (list_t *) malloc(sizeof(list_t));
    if (ll == NULL) {
        return ll;
    }

    ll->head = node_create(NULL);
    ll->tail = node_create(NULL);
    ll->head->next = ll->tail;
    ll->tail->prev = ll->head;
    ll->size = 0;
    return ll;
}

void ll_destroy(list_t **ll, void (*del_func_ptr)(void *)) {
    if (ll && *ll) {
        while ((*ll)->head != NULL) {
            node_t *del = (*ll)->head;
            (*ll)->head = (*ll)->head->next;
            node_destroy(&del, del_func_ptr);
        }

        free(*ll);
        *ll = NULL;
    }
}

size_t ll_size(list_t *ll) {
    return ll->size;
}

bool ll_empty(list_t *ll) {
    return ll->size == 0;
}

bool ll_push_front(list_t *ll, void *data) {
    if (ll == NULL) {
        return false;
    }

    node_t *n = node_create(data);
    n->prev = ll->head;
    n->next = ll->head->next;
    ll->head->next->prev = n;
    ll->head->next = n;
    ll->size += 1;
    return true;
}

bool ll_push_back(list_t *ll, void *data) {
    if (ll == NULL) {
        return false;
    }

    node_t *n = node_create(data);
    n->next = ll->tail;
    n->prev = ll->tail->prev;
    ll->tail->prev->next = n;
    ll->tail->prev = n;
    ll->size += 1;
    return true;
}

void *ll_pop_front(list_t *ll) {
    if (ll_empty(ll) == true) {
        return NULL;
    }

    node_t *pop = ll->head->next;
    void *data = pop->data;
    pop->next->prev = pop->prev;
    pop->prev->next = pop->next;
    node_destroy(&pop, NULL);
    ll->size -= 1;
    return data; // passing back ownership of data to user
}

void *ll_pop_back(list_t *ll) {
    if (ll_empty(ll) == true) {
        return NULL;
    }

    node_t *pop = ll->tail->prev;
    void *data = pop->data;
    pop->prev->next = pop->next;
    pop->next->prev = pop->prev;
    node_destroy(&pop, NULL);
    ll->size -= 1;
    return data; // passing back ownership of data to user
}
