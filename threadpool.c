#include "threadpool.h"
#include <stdlib.h>
#include <stdio.h>

threadpool_t *threadpool_create(int nthreads, void (*connection_func)(connection_t *)) {
    threadpool_t *tpool = (threadpool_t *) malloc(sizeof(threadpool_t));
    if (tpool == NULL) {
        return NULL;
    }

    tpool->wqueue = queue_create();
    if (tpool->wqueue == NULL) {
        free(tpool);
        return NULL;
    }

    // we like to live life on the edge here and forget about the error checking
    tpool->wqlock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    tpool->wqnotify = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

    tpool->connection_func = connection_func;
    tpool->nthreads = nthreads;
    tpool->shutdown = false;

    tpool->pool = (pthread_t *) calloc(nthreads, sizeof(pthread_t));
    if (tpool->pool == NULL) {
        queue_destroy(&tpool->wqueue, NULL);
        free(tpool);
        return NULL;
    }

    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&tpool->pool[i], NULL, free_young_thug, (void *) tpool) != 0) {
            threadpool_destroy(&tpool);
            return NULL;
        }
    }

    return tpool;
}

void threadpool_destroy(threadpool_t **tpool) {
    if (tpool == NULL) {
        return;
    }

    // lock and notify all threads that its time to mimis
    pthread_mutex_lock(&(*tpool)->wqlock);
    if ((*tpool)->shutdown == true) {
        return;
    }

    (*tpool)->shutdown = true;
    pthread_cond_broadcast(&(*tpool)->wqnotify); // hey uzi!! wake yo a** up!!
    pthread_mutex_unlock(&(*tpool)->wqlock);

    for (int i = 0; i < (*tpool)->nthreads; i++) {
        if ((*tpool)->pool[i] != 0) {
            pthread_join((*tpool)->pool[i], NULL);
        }
    }

    pthread_mutex_destroy(&(*tpool)->wqlock);
    pthread_cond_destroy(&(*tpool)->wqnotify);
    queue_destroy(&(*tpool)->wqueue, connection_destroy);
    free((*tpool)->pool);
    free(*tpool);

    *tpool = NULL;
}

bool threadpool_add_connection(threadpool_t *tpool, connection_t *conn) {
    if (tpool == NULL || conn == NULL) {
        return false;
    }

    pthread_mutex_lock(&tpool->wqlock);
    enqueue(tpool->wqueue, (void *) conn);
    pthread_cond_signal(&tpool->wqnotify);
    pthread_mutex_unlock(&tpool->wqlock);
    return true;
}

void threadpool_suspend_connection(threadpool_t *tpool, connection_t *conn) {
    if (tpool == NULL || conn == NULL) {
        return;
    }

    int flags = conn->req.reqline.method == GET ? EPOLLOUT : EPOLLIN;
    conn->req.status = OK;

    pthread_mutex_lock(&tpool->cmlock);
    redblack_insert(tpool->cmap, conn->connfd, conn);
    add_connection(tpool->cpoll, conn->connfd, flags);
    pthread_mutex_unlock(&tpool->cmlock);
}

void *free_young_thug(void *thread_pool_arg) {
    threadpool_t *tpool = (threadpool_t *) thread_pool_arg;
    connection_t *conn = NULL;

    while (true) {
        pthread_mutex_lock(&tpool->wqlock);
        while (dequeue(tpool->wqueue, (void *) &conn) == false && tpool->shutdown == false) {
            pthread_cond_wait(&tpool->wqnotify, &tpool->wqlock);
        }
        pthread_mutex_unlock(&tpool->wqlock);

        if (tpool->shutdown == true) {
            break;
        }

        tpool->connection_func(conn);

        if (conn->req.status == SUSPEND) {
            threadpool_suspend_connection(tpool, conn);
            continue;
        }

        connection_destroy(conn);
    }

    return (void *) NULL;
}
