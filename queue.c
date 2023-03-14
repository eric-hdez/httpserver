#include "linkedlist.h"
#include "queue.h"
#include <stdlib.h>

struct queue_t {
    list_t *items;
};

queue_t *queue_create(void) {
    queue_t *q = (queue_t *) malloc(sizeof(queue_t));
    if (q == NULL) {
        return q;
    }

    q->items = ll_create();
    return q;
}

void queue_destroy(queue_t **q, void (*del_func)(void *)) {
    if (q && *q) {
        ll_destroy(&(*q)->items, del_func);
        free(*q);
        *q = NULL;
    }
}

size_t queue_size(queue_t *q) {
    return ll_size(q->items);
}

bool queue_empty(queue_t *q) {
    return ll_empty(q->items);
}

bool enqueue(queue_t *q, void *data) {
    if (q == NULL) {
        return false;
    }

    return ll_push_front(q->items, data);
}

bool dequeue(queue_t *q, void **data) {
    if (q == NULL || queue_empty(q)) {
        return false;
    }

    *data = ll_pop_back(q->items);
    return true;
}
