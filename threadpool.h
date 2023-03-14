#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include "connection.h"
#include "queue.h"
#include "connpoll.h"
#include "redblack.h"
#include <pthread.h>
#include <stdbool.h>

typedef struct threadpool_t threadpool_t;

//typedef struct thread_task_t thread_task_t;

typedef redblack_t map_t; // a map structure using redblack trees :)

struct threadpool_t {
    queue_t *wqueue;
    map_t *cmap;
    pthread_t *pool;
    connpoll_t *cpoll;
    pthread_mutex_t wqlock;
    pthread_cond_t wqnotify;
    pthread_mutex_t cmlock;
    void (*connection_func)(connection_t *);
    int nthreads;
    bool shutdown;
};

struct thread_task_t {
    void (*worker_func)(void *);
    connection_t *task;
};

threadpool_t *threadpool_create(int nthreads, void (*connection_func)(connection_t *));

void threadpool_destroy(threadpool_t **pool);

bool threadpool_add_connection(threadpool_t *tpool, connection_t *conn);

void threadpool_suspend_connection(threadpool_t *tpool, connection_t *conn);

void *free_young_thug(void *thread_pool_arg);

#endif
