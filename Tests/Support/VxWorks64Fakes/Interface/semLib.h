/* A test stand-in for the VxWorks 6.4 semaphore library header.
 *
 * Supplies the subset of semLib.h that Platform/VxWorks64 calls, with the
 * prototypes of the public API reference. The semaphore itself is opaque to
 * callers, so the struct is declared and never defined.
 */
#ifndef SEMLIB_H
#define SEMLIB_H

#include "vxWorks.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct semaphore* SEM_ID;

    SEM_ID semMCreate(int options);

#ifdef __cplusplus
}
#endif

#endif /* SEMLIB_H */
