#ifndef __STATUS_H__
#define __STATUS_H__

#include <inttypes.h>

#define GET_OK_MSG    "HTTP/1.1 200 OK\r\nContent-Length: %" PRId64 "\r\n\r\n"
#define OK_MSG        "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n"
#define CREATED_MSG   "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n"
#define FORBIDDEN_MSG "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n"
#define NOT_FOUND_MSG "HTTP/1.1 404 File Not Found\r\nContent-Length: 15\r\n\r\nFile Not Found\n"
#define BAD_REQ_MSG   "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n"
#define INTERNAL_MSG                                                                               \
    "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal Server Error\n"
#define NOT_IMPL_MSG "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n"

#define GET_LOG_MSG    "GET,/%s,%d,%" PRIu32 "\n"
#define PUT_LOG_MSG    "PUT,/%s,%d,%" PRIu32 "\n"
#define APPEND_LOG_MSG "APPEND,/%s,%d,%" PRIu32 "\n"

typedef enum {
    CONN_CLOSED = -1,
    SUSPEND = 0,
    OK = 200,
    CREATED = 201,
    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    FILE_NOT_FOUND = 404,
    INT_ERR = 500,
    NOT_IMPL = 501
} status_t;

static inline char *resolve_status_msg(status_t status) {
    switch (status) {
    case OK: return OK_MSG;
    case CREATED: return CREATED_MSG;
    case BAD_REQUEST: return BAD_REQ_MSG;
    case FORBIDDEN: return FORBIDDEN_MSG;
    case FILE_NOT_FOUND: return NOT_FOUND_MSG;
    case INT_ERR: return INTERNAL_MSG;
    case NOT_IMPL: return NOT_IMPL_MSG;
    default: return BAD_REQ_MSG;
    }
}

#endif
