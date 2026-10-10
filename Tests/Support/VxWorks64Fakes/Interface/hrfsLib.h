/* A test stand-in for the VxWorks 6.4 HRFS library header.
 *
 * Supplies the subset of hrfsLib.h that the VxWorks 6.4 BDD target calls,
 * renamed to the file-system fake's. Relies on vxWorks.h having been included
 * first, as VxWorks does.
 */
#ifndef HRFSLIB_H
#define HRFSLIB_H

#define hrfsFormat VxWorks64FsFake_HrfsFormat

#ifdef __cplusplus
extern "C"
{
#endif

    STATUS hrfsFormat(char* path, UINT64 diskSize, UINT32 blkSize, UINT32 numInodes);

#ifdef __cplusplus
}
#endif

#endif /* HRFSLIB_H */
