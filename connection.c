#include "connection.h"
#include <stdlib.h>
#include <unistd.h>

connection_t *connection_create(void) {
    connection_t *conn = (connection_t *) calloc(1, sizeof(connection_t));
    if (conn == NULL) {
        return NULL;
    }

    conn->connfd = -1;
    conn->req = request_create();
    return conn;
}

void connection_destroy(void *conn) {
    connection_t *connptr = (connection_t *) conn;
    if (connptr != NULL) {
        close(connptr->connfd);
        request_destroy(&connptr->req);
        free(connptr);
    }
}
