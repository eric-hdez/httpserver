#ifndef __REDBLACK_H__
#define __REDBLACK_H__

#include "connection.h"
#include <stdbool.h>
#include <sys/types.h>

typedef struct rbnode_t rbnode_t;

typedef struct redblack_t redblack_t;

redblack_t *redblack_create(void);

void redblack_destroy(redblack_t **rbt);

void redblack_insert(redblack_t *rbt, int key, connection_t *value);

connection_t *redblack_extract(redblack_t *rbt, int key);

void redblack_delete(redblack_t *rbt, int key);

#endif
