#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdbool.h>
#include <sys/types.h>

typedef struct queue_t queue_t;

queue_t *queue_create(void);

void queue_destroy(queue_t **q, void (*del_func)(void *));

size_t queue_size(queue_t *q);

bool queue_empty(queue_t *q);

bool enqueue(queue_t *q, void *data);

bool dequeue(queue_t *q, void **data);

void queue_print(queue_t *q);

#endif
