#include "connection.h"
#include "debug.h"
#include "ioutil.h"
#include "request.h"
#include "status.h"
#include "util.h"
#include "queue.h"
#include "connpoll.h"
#include "redblack.h"
#include "threadpool.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define OPTIONS              "t:l:"
#define DEFAULT_THREAD_COUNT 4

static FILE *logfile;
#define LOG(...) fprintf(logfile, __VA_ARGS__);

threadpool_t *thread_pool;
map_t *connection_map;
pthread_mutex_t maplock;
connpoll_t *connection_poll;
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;

// Creates a socket for listening for connections.
// Closes the program and prints an error message on error.
//
// port: binding portnumber for connection listening
//
static int create_listen_socket(uint16_t port) {
    struct sockaddr_in addr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        err(EXIT_FAILURE, "socket error");
    }
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        err(EXIT_FAILURE, "bind error");
    }
    if (listen(listenfd, 128) < 0) {
        err(EXIT_FAILURE, "listen error");
    }
    return listenfd;
}

void log_request(request_t *req, status_t status) {
    switch (req->reqline.method) {
    case GET: LOG(GET_LOG_MSG, req->reqline.object, status, req->fields.reqid); break;
    case PUT: LOG(PUT_LOG_MSG, req->reqline.object, status, req->fields.reqid); break;
    case APPEND: LOG(APPEND_LOG_MSG, req->reqline.object, status, req->fields.reqid); break;
    default: break;
    }

    fflush(logfile);
}

// GET request handler for http server. Updates a status code througout
// the request to reflect its success or failure
//
// connfd: socket file description for client connection
// h     : pointer to http request header struct
// status: status code tracking request status
//
void handle_get(connection_t *conn) {
    if (conn->req.state == HANDLE_REQUEST) {
        pthread_mutex_lock(&file_lock);
        conn->req.object.fd = open_file(conn->req.reqline.object, O_RDONLY, &conn->req.status);
        if (conn->req.object.fd < 0) {
            log_request(&conn->req, conn->req.status);
            pthread_mutex_unlock(&file_lock);
            send_http_response(conn->connfd, &conn->req, conn->req.status);
            return;
        }

        log_request(&conn->req, conn->req.status);
        pthread_mutex_unlock(&file_lock);

        if (file_is_dir(conn->req.object.fd, &conn->req.status) != 0) {
            send_http_response(conn->connfd, &conn->req, conn->req.status);
            return;
        }

        conn->req.fields.contlen = sizeof_file(conn->req.object.fd, &conn->req.status);
        if (conn->req.fields.contlen < 0) {
            send_http_response(conn->connfd, &conn->req, conn->req.status);
            return;
        }

        conn->req.state = SEND_ACK;
    }

    // send OK to client before sending contents
    if (conn->req.state == SEND_ACK) {
        if (send_http_response(conn->connfd, &conn->req, conn->req.status) < 0) {
            return;
        }

        conn->req.fields.contlen = 0;
        conn->req.state = SEND_BODY;
    }

    if (conn->req.state == SEND_BODY) {
        if (send_http_body(conn->connfd, &conn->req) < 0) {
            if (conn->req.status == SUSPEND) {
                return;
            }

            send_http_response(conn->connfd, &conn->req, conn->req.status);
        }
    }
}

// PUT request handler for http server. Updates a status code througout
// the request to reflect its success or failure
//
// connfd: socket file description for client connection
// h     : pointer to http request header struct
// status: status code tracking request status
//
void handle_put(connection_t *conn) {
    if (conn->req.state == HANDLE_REQUEST) {
        int fd;

        fd = open_file(conn->req.reqline.object, O_APPEND | O_WRONLY, &conn->req.status);
        if (fd < 0) {
            if (conn->req.status == FILE_NOT_FOUND) {
                conn->req.object.status = CREATED;
            } else {
                send_http_response(conn->connfd, &conn->req, conn->req.status);
                return;
            }
        }

        if (conn->req.object.status != CREATED && file_is_dir(fd, &conn->req.status) != 0) {
            send_http_response(conn->connfd, &conn->req, conn->req.status);
            close(fd);
            return;
        }

        if (fd > 2) {
            close(fd);
        }
        conn->req.tmp.fd = create_tmpfile(conn->req.tmp.name, &conn->req.status);
        conn->req.state = RECV_REM_BODY;
    }

    if (conn->req.state == RECV_REM_BODY) {
        recv_rem_http_body(&conn->req);
    }

    if (conn->req.state == RECV_BODY) {
        if (recv_http_body(conn->connfd, &conn->req) < 0) {
            if (conn->req.status == SUSPEND) {
                return;
            }

            send_http_response(conn->connfd, &conn->req, conn->req.status);
            return;
        }
    }

    if (conn->req.state == WRITE_BODY) {
        pthread_mutex_lock(&file_lock);
        if (unlink(conn->req.reqline.object) == 0) {
            conn->req.object.status = OK;
        }
        rename(conn->req.tmp.name, conn->req.reqline.object);
        log_request(&conn->req, conn->req.object.status);
        pthread_mutex_unlock(&file_lock);
        conn->req.state = DONE;
    }

    if (conn->req.state == DONE) {
        send_http_response(conn->connfd, &conn->req, conn->req.object.status);
    }
}

// APPEND request handler for http server. Updates a status code througout
// the request to reflect its success or failure
//
// connfd: socket file description for client connection
// h     : pointer to http request header struct
// status: status code tracking request status
//
void handle_append(connection_t *conn) {
    int fd;

    if (conn->req.state == HANDLE_REQUEST) {
        pthread_mutex_lock(&file_lock);
        fd = open_file(conn->req.reqline.object, O_APPEND | O_WRONLY, &conn->req.status);
        if (fd < 0) {
            log_request(&conn->req, conn->req.status);
            pthread_mutex_unlock(&file_lock);
            send_http_response(conn->connfd, &conn->req, conn->req.status);
            return;
        }
        pthread_mutex_unlock(&file_lock);

        if (file_is_dir(fd, &conn->req.status) != 0) {
            send_http_response(conn->connfd, &conn->req, conn->req.status);
            log_request(&conn->req, conn->req.status);
            close(fd);
            return;
        }

        if (fd > 2) {
            close(fd);
        }
        conn->req.tmp.fd = create_tmpfile(conn->req.tmp.name, &conn->req.status);
        unlink(conn->req.tmp.name);
        conn->req.object.size = conn->req.fields.contlen;
        conn->req.state = RECV_REM_BODY;
    }

    if (conn->req.state == RECV_REM_BODY) {
        recv_rem_http_body(&conn->req);
    }

    if (conn->req.state == RECV_BODY) {
        if (recv_http_body(conn->connfd, &conn->req) < 0) {
            if (conn->req.status == SUSPEND) {
                return;
            }

            send_http_response(conn->connfd, &conn->req, conn->req.status);
            return;
        }
    }

    //uint8_t buffer[BLOCK] = { 0 };
    //int nbytes = 0;

    if (conn->req.state == WRITE_BODY) {
        pthread_mutex_lock(&file_lock);
        conn->req.object.fd = open_file(conn->req.reqline.object, O_WRONLY, &conn->req.status);
        if (conn->req.object.fd < 0) {
            pthread_mutex_unlock(&file_lock);
            send_http_response(conn->connfd, &conn->req, conn->req.status);
            return;
        }

        append_file(conn->req.tmp.fd, conn->req.object.fd, conn->req.object.size);

        log_request(&conn->req, conn->req.status);
        pthread_mutex_unlock(&file_lock);
        conn->req.state = DONE;
    }

    if (conn->req.state == DONE) {
        send_http_response(conn->connfd, &conn->req, conn->req.status);
    }
}

void handle_connection(connection_t *conn) {
    if (conn->req.state == RECV_HEADER) {
        recv_http_request(conn->connfd, &conn->req);
    }

    if (conn->req.state == PARSE_HEADER) {
        parse_http_request(&conn->req);
    }

    if (conn->req.status == OK && conn->req.state != DONE) {
        switch (conn->req.reqline.method) {
        case GET: handle_get(conn); break;
        case PUT: handle_put(conn); break;
        case APPEND: handle_append(conn); break;
        default: break;
        }
    } else if (conn->req.status != SUSPEND) {
        send_http_response(conn->connfd, &conn->req, conn->req.status);
        log_request(&conn->req, conn->req.status);
    }
}

static void sigterm_handler(int sig) {
    if (sig == SIGTERM) {
        warnx("received SIGTERM");
        fclose(logfile);
        threadpool_destroy(&thread_pool);
        redblack_destroy(&connection_map);
        connpoll_destroy(&connection_poll);
        pthread_mutex_destroy(&maplock);
        exit(EXIT_SUCCESS);
    }
}

static void usage(char *exec) {
    fprintf(stderr, "usage: %s [-t threads] [-l logfile] <port>\n", exec);
}

int main(int argc, char *argv[]) {
    int opt = 0;
    int threads = DEFAULT_THREAD_COUNT;
    logfile = stderr;

    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 't':
            threads = strtol(optarg, NULL, 10);
            if (threads <= 0) {
                errx(EXIT_FAILURE, "bad number of threads");
            }
            break;
        case 'l':
            logfile = fopen(optarg, "w");
            if (!logfile) {
                errx(EXIT_FAILURE, "bad logfile");
            }
            break;
        default: usage(argv[0]); return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        warnx("wrong number of arguments");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    uint16_t port = strtouint16(argv[optind]);
    if (port == 0) {
        errx(EXIT_FAILURE, "bad port number: %s", argv[1]);
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sigterm_handler);

    int listenfd = create_listen_socket(port);

    connection_poll = connpoll_create(4096);
    connection_map = redblack_create();
    maplock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    thread_pool = threadpool_create(threads, handle_connection);
    thread_pool->cmap = connection_map;
    thread_pool->cmlock = maplock;
    thread_pool->cpoll = connection_poll;

    add_connection(connection_poll, listenfd, EPOLLIN);

    for (;;) {
        poll_connections(connection_poll);

        int connfd;
        connection_t *conn;

        while (yield_connection(connection_poll, &connfd) == true) {
            if (connfd == listenfd) {
                int connfd = accept(listenfd, NULL, NULL);
                conn = connection_create();
                conn->connfd = connfd;
            } else {
                pthread_mutex_lock(&maplock);
                conn = redblack_extract(connection_map, connfd);
                delete_connection(connection_poll, connfd);
                pthread_mutex_unlock(&maplock);
            }

            threadpool_add_connection(thread_pool, conn);
        }
    }

    return EXIT_SUCCESS;
}
