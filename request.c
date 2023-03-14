#include "ioutil.h"
#include "request.h"
#include "debug.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define WAIT_TIME 100

// initializes an http header struct and its members
//
request_t request_create(void) {
    request_t req = {
        .header = { 0 },
        .reqline = { 0 },
        .fields = { 0, -1 },
        .object = { -1, 0, OK },
        .tmp = { -1, { 0 } },
        .status = OK,
        .state = RECV_HEADER,
    };
    //request_t req = { { 0 }, { 0 }, { 0, -1 } };
    return req;
}

// de-initializes an http header struct by deallocating its member's
// heap memory
//
// h: pointer to header struct
//
void request_destroy(request_t *req) {
    if (req->reqline.object != NULL) {
        free(req->reqline.object);
    }

    if (req->reqline.version != NULL) {
        free(req->reqline.version);
    }

    if (req->tmp.fd > 2) {
        close(req->tmp.fd);
    }

    if (req->object.fd > 2) {
        close(req->object.fd);
    }
}

// recieves an http request from a socket until it has been
// recieved fully, or the client closes connection. returns
// the number of bytes read or < 0 if there was an error
//
// connfd: socket file descriptor
// buffer: byte buffer for socket content
// status: status code tracking request status
//
void recv_http_request(int connfd, request_t *req) {
    char *endcrlf = "\r\n\r\n";
    ssize_t nbytes = 0;

    do {
        nbytes = recv(
            connfd, req->header.buf + req->header.size, REQSIZE - req->header.size, MSG_DONTWAIT);
        if (nbytes < 0) {
            switch (errno) {
            case EWOULDBLOCK: req->status = SUSPEND; return;
            default:
                req->status = INT_ERR;
                req->state = DONE;
                return;
            }
        }

        req->header.size += nbytes;
    } while (nbytes > 0 && strcontains((char *) req->header.buf, endcrlf) == false);

    req->status = OK;
    req->state = PARSE_HEADER;
}

int64_t recv_rem_http_body(request_t *req) {
    ssize_t nbytes = 0;

    if (req->fields.contlen <= 0) {
        req->status = OK;
        req->state = WRITE_BODY;
        return nbytes;
    }

    if (req->header.rembytes > 0) {
        nbytes = write_bytes(req->tmp.fd, req->header.reqeo, req->header.rembytes);
        req->fields.contlen -= nbytes;

        if (req->fields.contlen <= 0) {
            req->status = OK;
            req->state = WRITE_BODY;
            return nbytes;
        }
    }

    req->status = OK;
    req->state = RECV_BODY;
    return nbytes;
}

// recieves the body of an http request from a socket until it has
// been received fully, or the client closes connection. updates
// status code if internal server error is encountered
//
// connfd: socket file descriptor
// h     : pointer to header struct
// fd    : file descriptor of file where body is written
// status: status code tracking request status
//
int64_t recv_http_body(int connfd, request_t *req) {
    uint8_t buffer[BLOCK] = { 0 };
    ssize_t nbytes = 0;

    do {
        nbytes = recv(connfd, buffer, BLOCK, MSG_DONTWAIT);
        if (nbytes < 0) {
            switch (errno) {
            case EWOULDBLOCK: req->status = SUSPEND; return nbytes;
            default:
                req->status = INT_ERR;
                req->state = DONE;
                return nbytes;
            }
        }

        nbytes = req->fields.contlen < nbytes ? req->fields.contlen : nbytes;
        write_bytes(req->tmp.fd, buffer, nbytes);
        req->fields.contlen -= nbytes;
    } while (req->fields.contlen > 0 && nbytes > 0);

    req->status = OK;
    req->state = WRITE_BODY;
    return nbytes;
}

// sends the body (contents) for an http request to the client
// socket until it has been fully recieved, or the client closes
// the connection. updates status code in internal server error
// or bad request errors are encountered
//
// connfd: socket file descriptor
// fd    : requested file's file descriptor
// status: status code tracking request status
//
int64_t send_http_body(int connfd, request_t *req) {
    uint8_t buffer[BLOCK] = { 0 };
    ssize_t nbytes = 0, sbytes = 0;

    // take precaution to set contlen to 0 before calling this function
    do {
        nbytes = read_bytes(req->object.fd, buffer, BLOCK);
        sbytes = send(connfd, buffer, nbytes, MSG_DONTWAIT);
        if (sbytes < 0) {
            switch (errno) {
            case EWOULDBLOCK:
                lseek(req->object.fd, req->fields.contlen, SEEK_SET);
                req->status = SUSPEND;
                return sbytes;
            case EPIPE:
                req->status = CONN_CLOSED;
                req->state = DONE;
                return sbytes;
            case ECONNRESET:
                req->status = CONN_CLOSED;
                req->state = DONE;
                return sbytes;
            default:
                req->status = INT_ERR;
                req->state = DONE;
                return sbytes;
            }
        }

        req->fields.contlen += nbytes;
    } while (nbytes > 0 && sbytes > 0);

    req->status = OK;
    req->state = DONE;
    return req->fields.contlen;
}

// sends an http response to the client socket. If the send fails,
// status is updated accordingly to reflect a bad request or an
// internal server error
//
// connfd: socket file descriptor
// h     : pointer to header struct
// status: status code tracking request status
//
ssize_t send_http_response(int connfd, request_t *req, status_t status) {
    char buffer[BLOCK] = { 0 };
    ssize_t nbytes = 0;
    char *msg;

    if (status == OK && req->reqline.method == GET) {
        sprintf(buffer, GET_OK_MSG, req->fields.contlen);
        msg = buffer;
    } else {
        msg = resolve_status_msg(status);
    }

    nbytes = send(connfd, msg, strlen(msg), 0);
    if (nbytes < 0) {
        switch (errno) {
        case EPIPE:
            req->status = CONN_CLOSED;
            req->state = DONE;
            return nbytes;
        case ECONNRESET:
            req->status = CONN_CLOSED;
            req->state = DONE;
            return nbytes;
        default:
            req->status = INT_ERR;
            req->state = DONE;
            return nbytes;
        }
    }

    return nbytes;
}

// parses an http header and populates a header struct with the fields
// of the request
//
// h           : pointer to header struct
// buffer      : buffer containing http header
// header_bytes: size of the http header in bytes (+ content body)
//
void parse_http_request(request_t *req) {
    header_re_t h_reg;

    if (compile_header_re(&h_reg) != 0) {
        req->status = INT_ERR;
        req->state = DONE;
        return;
    }

    regmatch_t match[5] = { 0 };

    if (regexec(&h_reg.reql_re, (char *) req->header.buf, 5, match, 0) == REG_NOMATCH) {
        req->status = BAD_REQUEST;
        req->state = DONE;
        free_header_re(&h_reg);
        return;
    }

    // check the method of the request -------------------------------------------------------------
    size_t method_len = match[1].rm_eo - match[1].rm_so;
    char *method = strndup((char *) req->header.buf, method_len);

    if (strcmp(method, "GET") == 0 || strcmp(method, "get") == 0) {
        req->reqline.method = GET;
    } else if (strcmp(method, "PUT") == 0 || strcmp(method, "put") == 0) {
        req->reqline.method = PUT;
    } else if (strcmp(method, "APPEND") == 0 || strcmp(method, "append") == 0) {
        req->reqline.method = APPEND;
    } else {
        req->status = NOT_IMPL;
        req->state = DONE;
        free_header_re(&h_reg);
        free(method);
        return;
    }

    free(method);

    // check the object of the request -------------------------------------------------------------
    size_t object_len = match[2].rm_eo - match[2].rm_so;

    if (object_len > 19) {
        req->status = BAD_REQUEST;
        req->state = DONE;
        free_header_re(&h_reg);
        return;
    }

    req->reqline.object = strndup((char *) req->header.buf + match[2].rm_so, object_len);

    // if no REG_NOMATCH, then version is good -----------------------------------------------------
    size_t version_len = match[3].rm_eo - match[3].rm_so;
    req->reqline.version = strndup((char *) req->header.buf + match[3].rm_so, version_len);

    // check for end of request, no header fields --------------------------------------------------
    if (match[4].rm_so > 0) {
        if (req->reqline.method != GET) {
            req->status = BAD_REQUEST;
            req->state = DONE;
        } else {
            req->status = OK;
            req->state = HANDLE_REQUEST;
        }

        free_header_re(&h_reg);
        return;
    }

    // check the field lines of the request --------------------------------------------------------
    uint8_t *fieldptr = req->header.buf + match[0].rm_eo;

    do {
        if (regexec(&h_reg.field_re, (char *) fieldptr, 4, match, 0) == REG_NOMATCH) {
            req->status = BAD_REQUEST;
            req->state = DONE;
            free_header_re(&h_reg);
            return;
        }

        if (strncmp((char *) fieldptr, "Content-Length", 14) == 0) {
            size_t len_len = match[2].rm_eo - match[2].rm_so;
            char *num = strndup((char *) fieldptr + match[2].rm_so, len_len);
            req->fields.contlen = strtoint64u(num);
            free(num);
        }

        if (strncmp((char *) fieldptr, "Request-Id", 10) == 0) {
            size_t id_len = match[2].rm_eo - match[2].rm_so;
            char *num = strndup((char *) fieldptr + match[2].rm_so, id_len);
            req->fields.reqid = strtouint32(num);
            free(num);
        }

        fieldptr += match[0].rm_eo;
    } while (match[3].rm_so < 0);

    if (req->fields.contlen < 0 && req->reqline.method != GET) {
        req->status = BAD_REQUEST;
        req->state = DONE;
        free_header_re(&h_reg);
        return;
    }

    if (regexec(&h_reg.end_re, (char *) req->header.buf, 2, match, 0) == REG_NOMATCH) {
        req->status = BAD_REQUEST;
        req->state = DONE;
        free_header_re(&h_reg);
        return;
    }

    // check end of request for extra body bytes ---------------------------------------------------
    req->header.reqeo = req->header.buf + match[0].rm_eo;
    req->header.rembytes = req->header.size - match[0].rm_eo;
    req->header.rembytes
        = req->fields.contlen < req->header.rembytes ? req->fields.contlen : req->header.rembytes;
    req->header.remout = req->header.rembytes > 0 ? true : false;

    req->status = OK;
    req->state = HANDLE_REQUEST;
    free_header_re(&h_reg);
}
