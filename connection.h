#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include "request.h"
#include "string.h"

typedef struct {
    int connfd;
    request_t req;
} connection_t;

connection_t *connection_create(void);

void connection_destroy(void *conn);

#endif
