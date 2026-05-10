#ifndef MINSIZE_H
#define MINSIZE_H

#include <stddef.h>

static inline size_t MinSize(size_t a, size_t b)
{
    return (a < b) ? a : b;
}

#endif /* MINSIZE_H */
