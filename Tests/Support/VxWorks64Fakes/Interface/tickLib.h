/* A test stand-in for the VxWorks 6.4 tick library header.
 *
 * Supplies the subset of tickLib.h that Platform/VxWorks64 calls, with the
 * prototype of the public API reference.
 */
#ifndef TICKLIB_H
#define TICKLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

    UINT64 tick64Get(void);

#ifdef __cplusplus
}
#endif

#endif /* TICKLIB_H */
