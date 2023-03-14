#define _GNU_SOURCE

#include "ioutil.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// wrapper function for syscall read() that ensures the total
// number of specified bytes gets read without failure
//
// fd    : input file's file descriptor
// buffer: byte buffer for file contents
// nbytes: number of bytes requested from file
//
// * borrowed and modified from my own work at:
//   https://github.com/eric-hdez/huffman-compression/blob/main/io.c *
//
ssize_t read_bytes(int fd, uint8_t *buffer, size_t nbytes) {
    ssize_t rbytes = -1;
    size_t total = 0;

    while (total != nbytes && rbytes != 0) {
        rbytes = read(fd, buffer, nbytes - total);
        total += rbytes, buffer += rbytes;
    }

    return total;
}

// wrapper function for syscall write() that ensures the total
// number of specified bytes gets read without failure
//
// fd    : output file's file descriptor
// buffer: byte buffer for output contents
// nbytes: number of bytes outputted to file
//
// * borrowed and modified from my own work at:
//   https://github.com/eric-hdez/huffman-compression/blob/main/io.c *
//
ssize_t write_bytes(int fd, uint8_t *buffer, size_t nbytes) {
    ssize_t wbytes = -1;
    size_t total = 0;

    while (total != nbytes && wbytes != 0) {
        wbytes = write(fd, buffer, nbytes - total);
        total += wbytes, buffer += wbytes;
    }

    return total;
}

// wrapper function for syscall open() that returns a
// status pertaining to the success of the file open
//
// filename: name of the file
// flags   : open flags
// status  : status pertaining to an http response
//
int open_file(char filename[], int flags, status_t *status) {
    int fd;

    if ((fd = open(filename, flags, 0600)) < 0) {
        if (errno == ENOENT) {
            *status = FILE_NOT_FOUND;
        } else if (errno == EACCES) {
            *status = FORBIDDEN;
        } else {
            *status = BAD_REQUEST;
        }
    }

    return fd;
}

// wrapper function for syscall open() (with O_CREAT)
// that returns a status pertaining to the success
// of the file open
//
// filename: name of the file
// status  : status pertaining to an http response
//
int create_file(char filename[], status_t *status) {
    int fd;

    if ((fd = creat(filename, 0600)) < 0) {
        if (errno == ENOENT) {
            *status = FILE_NOT_FOUND;
        } else if (errno == EACCES) {
            *status = FORBIDDEN;
        } else {
            *status = BAD_REQUEST;
        }
    }

    *status = CREATED;
    return fd;
}

ssize_t append_file(int infd, int outfd, ssize_t nbytes) {
    ssize_t abytes = 0, total = 0;
    lseek(infd, 0, SEEK_SET);
    lseek(outfd, 0, SEEK_END);

    do {
        abytes = copy_file_range(infd, NULL, outfd, NULL, nbytes - total, 0);
        total += abytes;
    } while (total != nbytes && abytes > 0);

    return total;
}

// checks if a file is a directory, returns 1 if
// file is a directory, -1 if there was an error,
// and 0 f file is not a directory
//
// fd    : file's file descriptor
// status: status pertaining to an http response
//
int file_is_dir(int fd, status_t *status) {
    struct stat statbuf;
    if (fstat(fd, &statbuf) < 0) {
        if (errno == EACCES) {
            *status = FORBIDDEN;
        } else {
            *status = INT_ERR;
        }

        return -1;
    }

    if (S_ISDIR(statbuf.st_mode) != 0) {
        *status = FORBIDDEN;
        return 1;
    }

    return 0;
}

// returns the size of a file or -1 if the operation
// fails
//
// fd    : file's file descriptor
// status: status pertaining to an http response
//
int64_t sizeof_file(int fd, status_t *status) {
    struct stat statbuf;
    if (fstat(fd, &statbuf) < 0) {
        if (errno == EACCES) {
            *status = FORBIDDEN;
        } else {
            *status = INT_ERR;
        }

        return -1;
    }

    return statbuf.st_size;
}

int create_tmpfile(char *tmpname, status_t *status) {
    char filename[] = "tmpfileXXXXXX";
    size_t len = strlen(filename);

    int tmpfd = mkstemp(filename);
    if (tmpfd < 0) {
        *status = INT_ERR;
        return -1;
    }

    memcpy(tmpname, filename, len);
    tmpname[len] = '\0';

    return tmpfd;
}
