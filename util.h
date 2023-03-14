#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdbool.h>
#include <inttypes.h>

#define BLOCK 4096

uint16_t strtouint16(char num[]);

uint32_t strtouint32(char num[]);

int64_t strtoint64u(char num[]);

void strlower(char str[]);

bool strcontains(char *str, char *sequence);

#endif
