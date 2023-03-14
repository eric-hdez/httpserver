#include "util.h"
#include <stdlib.h>
#include <string.h>

// converts a string to a 16 bits unsigned integer or returns
// 0 if the string is malformed or out of the range.
//
// number: the number string
//
uint16_t strtouint16(char number[]) {
    char *last;
    long num = strtol(number, &last, 10);
    if (num <= 0 || num > UINT16_MAX || *last != '\0') {
        return 0;
    }

    return num;
}

// converts a string to a 32 bits unsigned integer or returns
// 0 if the string is malformed or out of the range.
//
// number: the number string
//
uint32_t strtouint32(char number[]) {
    char *last;
    int64_t num = strtoll(number, &last, 10);
    if (num <= 0 || num > UINT32_MAX || *last != '\0') {
        return 0;
    }

    return num;
}

// converts a string to an integer in the unsigned range of an
// int64_t integer or returns INT64_MIN if the string is
// malformed
//
// number: the number string
//
int64_t strtoint64u(char number[]) {
    char *last;
    int64_t num = strtoll(number, &last, 10);
    if (num < 0 || *last != '\0') {
        return INT64_MIN;
    }

    return num;
}

// converts a string into its lowercase representation by
// directly mutating the string
//
// str: input string
//
void strlower(char str[]) {
    for (; *str != '\0'; str++) {
        if (*str >= 'A' && *str <= 'Z') {
            *str = *str + ('a' - 'A');
        }
    }
}

// checks if a sequence of characters is contained in a string
// and returns true if it does, false otherwise
//
// str     : the haystack string
// sequence: the needle in the string
//
bool strcontains(char *str, char *sequence) {
    return strstr(str, sequence) != NULL;
}
