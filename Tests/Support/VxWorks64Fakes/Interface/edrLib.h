/* A test stand-in for the VxWorks 6.4 error detection and reporting header.
 *
 * Supplies the subset of edrLib.h that the VxWorks 6.4 BDD target calls. Relies
 * on vxWorks.h having been included first, as VxWorks does.
 */
#ifndef EDRLIB_H
#define EDRLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

    BOOL edrSystemDebugModeGet(void);
    void edrSystemDebugModeSet(BOOL mode);

#ifdef __cplusplus
}
#endif

#endif /* EDRLIB_H */
