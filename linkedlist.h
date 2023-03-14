#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#include <stdbool.h>
#include <sys/types.h>

typedef struct node_t node_t;

typedef struct list_t list_t;

struct node_t {
    void *data;
    node_t *next, *prev;
};

list_t *ll_create(void);

void ll_destroy(list_t **ll, void (*del_func_ptr)(void *));

size_t ll_size(list_t *ll);

bool ll_empty(list_t *ll);

bool ll_push_front(list_t *ll, void *data);

bool ll_push_back(list_t *ll, void *data);

void *ll_pop_front(list_t *ll);

void *ll_pop_back(list_t *ll);

#endif
