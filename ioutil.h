#ifndef __IOUTIL_H__
#define __IOUTIL_H__

#include "status.h"
#include "util.h"
#include <sys/types.h>

// * borrowed and modified from my own work at:
//   https://github.com/eric-hdez/huffman-compression/blob/main/io.c *
ssize_t read_bytes(int fd, uint8_t buffer[], size_t bytes);

// * borrowed and modified from my own work at:
//   https://github.com/eric-hdez/huffman-compression/blob/main/io.c *
ssize_t write_bytes(int fd, uint8_t buffer[], size_t bytes);

int open_file(char filename[], int flags, status_t *status);

int create_file(char filename[], status_t *status);

ssize_t append_file(int infd, int outfd, ssize_t nbytes);

int file_is_dir(int fd, status_t *status);

int64_t sizeof_file(int fd, status_t *status);

int create_tmpfile(char *tmpname, status_t *status);

#endif
