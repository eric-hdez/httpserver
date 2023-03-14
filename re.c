#include "re.h"

#define REQL_RE   "^([A-Za-z]+)\\s+/([a-zA-Z0-9._]+)\\s+(HTTP/1.1)\r\n(\r\n)?"
#define FIELDS_RE "^([^\r\n]+):\\s+([^\r\n]+)\r\n(\r\n)?"
#define END_RE    "\r\n\r\n"

// compiles regular expressions for parsing http headers and returns
// 0 upon success, -1 upon failure
//
// re: http header regex struct
//
int compile_header_re(header_re_t *re) {
    if (regcomp(&re->reql_re, REQL_RE, REG_EXTENDED)) {
        return -1;
    }

    if (regcomp(&re->end_re, END_RE, REG_EXTENDED)) {
        regfree(&re->reql_re);
        return -1;
    }

    if (regcomp(&re->field_re, FIELDS_RE, REG_EXTENDED)) {
        regfree(&re->reql_re);
        regfree(&re->end_re);
        return -1;
    }

    return 0;
}

// deallocates memory allocated for http header regular expressions
//
// re: http header regex struct
//
void free_header_re(header_re_t *re) {
    regfree(&re->reql_re);
    regfree(&re->end_re);
    regfree(&re->field_re);
}
