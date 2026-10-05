/* A test stand-in for the VxWorks 6.4 dosFs library header.
 *
 * Supplies the subset of dosFsLib.h that the VxWorks 6.4 BDD target calls,
 * renamed to the file-system fake's. Relies on vxWorks.h having been included
 * first, as VxWorks does.
 */
#ifndef DOSFSLIB_H
#define DOSFSLIB_H

#define dosFsVolFormat VxWorks64FsFake_DosFsVolFormat

#ifdef __cplusplus
extern "C"
{
#endif

    STATUS dosFsVolFormat(char* path, int opt, FUNCPTR pPromptFunc);

#ifdef __cplusplus
}
#endif

#endif /* DOSFSLIB_H */
