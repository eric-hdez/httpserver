#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "re.h"
#include "status.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define REQSIZE 2048
#define TMPSIZE 14

typedef enum { NONE, GET, PUT, APPEND } method_t;

typedef enum {
    RECV_HEADER,
    PARSE_HEADER,
    HANDLE_REQUEST,
    OPEN_FILE,
    SEND_ACK,
    SEND_BODY,
    RECV_REM_BODY,
    RECV_BODY,
    WRITE_BODY,
    DONE
} state_t;

typedef struct {
    method_t method;
    char *object;
    char *version;
} reqline_t;

typedef struct {
    uint32_t reqid;
    int64_t contlen;
} fields_t;

typedef struct {
    uint8_t buf[REQSIZE];
    uint8_t *reqeo;
    uint32_t size;
    int64_t rembytes;
    bool remout;
} header_t;

typedef struct {
    int fd;
    size_t size;
    status_t status;
} object_t;

typedef struct {
    int fd;
    char name[TMPSIZE];
} temp_t;

typedef struct {
    header_t header;
    reqline_t reqline;
    fields_t fields;
    object_t object;
    temp_t tmp;
    status_t status;
    state_t state;
} request_t;

request_t request_create(void);

void request_destroy(request_t *req);

void recv_http_request(int connfd, request_t *req);

int64_t recv_rem_http_body(request_t *req);

int64_t recv_http_body(int connfd, request_t *req);

int64_t send_http_body(int connfd, request_t *req);

ssize_t send_http_response(int connfd, request_t *req, status_t status);

void parse_http_request(request_t *req);

#endif
