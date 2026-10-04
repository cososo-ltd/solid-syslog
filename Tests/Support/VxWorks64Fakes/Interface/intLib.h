/* A test stand-in for the VxWorks 6.4 interrupt library header.
 *
 * Supplies the subset of intLib.h that Platform/VxWorks64 calls, with the
 * prototypes of the public API reference.
 *
 * It does not include vxWorks.h, for the reason semLib.h gives.
 */
#ifndef INTLIB_H
#define INTLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

    int intLock(void);
    int intUnlock(int lockKey);

#ifdef __cplusplus
}
#endif

#endif /* INTLIB_H */
