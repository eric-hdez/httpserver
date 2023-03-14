#ifndef __RE_H__
#define __RE_H__

#include <regex.h>

typedef struct {
    regex_t reql_re;
    regex_t field_re;
    regex_t end_re;
} header_re_t;

int compile_header_re(header_re_t *re);

void free_header_re(header_re_t *re);

#endif
