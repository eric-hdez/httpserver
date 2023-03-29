#include "connpoll.h"
#include <stdlib.h>
#include <unistd.h>

connpoll_t *connpoll_create(ssize_t size) {
    if (size < 0) {
        return NULL;
    }

    connpoll_t *cpoll = (connpoll_t *) malloc(sizeof(connpoll_t));
    if (!cpoll) {
        return NULL;
    }

    cpoll->epollfd = epoll_create1(0);
    if (cpoll->epollfd < 0) {
        free(cpoll);
        return NULL;
    }

    cpoll->events = (struct epoll_event *) calloc(size, sizeof(struct epoll_event));
    if (!cpoll->events) {
        free(cpoll);
        return NULL;
    }

    cpoll->readyfds = cpoll->iter = cpoll->size = 0;
    return cpoll;
}

void connpoll_destroy(connpoll_t **cpoll) {
    if (*cpoll && cpoll) {
        close((*cpoll)->epollfd);
        free((*cpoll)->events);
        free(*cpoll);
        *cpoll = NULL;
    }
}

bool add_connection(connpoll_t *cpoll, int connfd, int flags) {
    struct epoll_event ev = { 0 };
    ev.events = flags;
    ev.data.fd = connfd;

    int status = epoll_ctl(cpoll->epollfd, EPOLL_CTL_ADD, connfd, &ev);
    if (status < 0) {
        return false;
    }

    cpoll->size += 1;
    return true;
}

bool delete_connection(connpoll_t *cpoll, int connfd) {
    int status = epoll_ctl(cpoll->epollfd, EPOLL_CTL_DEL, connfd, NULL);
    if (status < 0) {
        return false;
    }

    cpoll->size -= 1;
    return true;
}

ssize_t poll_connections(connpoll_t *cpoll) {
    cpoll->iter = 0;
    return (cpoll->readyfds = epoll_wait(cpoll->epollfd, cpoll->events, cpoll->size, -1));
}

bool yield_connection(connpoll_t *cpoll, int *connfd) {
    if (cpoll->iter < cpoll->readyfds) {
        *connfd = cpoll->events[cpoll->iter++].data.fd;
        return true;
    }

    return false;
}
