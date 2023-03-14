#ifndef __SOCK_POLL_H__
#define __SOCK_POLL_H__

#include "connection.h"
#include <stdbool.h>
#include <sys/epoll.h>

typedef struct {
    int epollfd;
    struct epoll_event *events;
    ssize_t size, iter, readyfds;
} connpoll_t;

connpoll_t *connpoll_create(ssize_t size);

void connpoll_destroy(connpoll_t **cpoll);

bool add_connection(connpoll_t *cpoll, int connfd, int flags);

ssize_t poll_connections(connpoll_t *cpoll);

bool yield_connection(connpoll_t *cpoll, int *connfd);

bool delete_connection(connpoll_t *cpoll, int connfd);

#endif
