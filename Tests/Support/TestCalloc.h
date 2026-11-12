#ifndef TESTCALLOC_H
#define TESTCALLOC_H

#include <stdio.h>
#include <stdlib.h>

/* calloc for test doubles. A run that cannot allocate a double stops there,
 * saying so, instead of crashing later on its NULL. */
static inline void* TestCalloc(size_t size)
{
    void* allocated = calloc(1U, size);
    if (allocated == NULL)
    {
        (void) fprintf(stderr, "TestCalloc: out of memory allocating %zu bytes\n", size);
        abort();
    }
    return allocated;
}

#endif /* TESTCALLOC_H */
