#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>

#ifdef DEBUG_ENABLED
#define DEBUG(msg)                                                                                 \
    {                                                                                              \
        fprintf(stderr, "%s: line %d: " msg "\n", __FILE__, __LINE__);                             \
        fflush(stderr);                                                                            \
    }
#else
#define DEBUG(msg) // nothing
#endif

#endif
