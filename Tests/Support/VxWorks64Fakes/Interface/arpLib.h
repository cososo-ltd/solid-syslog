/* A test stand-in for the VxWorks 6.4 ARP library header.
 *
 * Supplies the subset of arpLib.h that Platform/VxWorks64 calls, with the
 * prototype of the kernel's header: neither string is const-qualified.
 *
 * It does not include vxWorks.h, for the reason semLib.h gives.
 */
#ifndef ARPLIB_H
#define ARPLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

/* A #define because it is one in the real header, as (M_arpLib | 2) with
 * M_arpLib being (71 << 16). The suppression lives here for the reason
 * semLib.h gives. */
/* NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */
#define S_arpLib_INVALID_HOST 0x00470002
    /* NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */

    STATUS arpResolve(char* targetAddr, char* pHwAddr, int numTries, int numTicks);

#ifdef __cplusplus
}
#endif

#endif /* ARPLIB_H */
