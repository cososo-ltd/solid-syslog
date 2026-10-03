/* A test stand-in for the VxWorks 6.4 system library header.
 *
 * Supplies the subset of sysLib.h that the BDD target calls, with the
 * prototype of the public API reference.
 */
#ifndef SYSLIB_H
#define SYSLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

    int sysClkRateGet(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSLIB_H */
