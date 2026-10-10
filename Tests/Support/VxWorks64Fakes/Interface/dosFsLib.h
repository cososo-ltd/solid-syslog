/* A test stand-in for the VxWorks 6.4 dosFs library header.
 *
 * Supplies the subset of dosFsLib.h that the VxWorks 6.4 BDD target calls,
 * renamed to the file-system fake's. Relies on vxWorks.h having been included
 * first, as VxWorks does.
 */
#ifndef DOSFSLIB_H
#define DOSFSLIB_H

#define dosFsVolFormat VxWorks64FsFake_DosFsVolFormat
#define dosFsVolDescGet VxWorks64FsFake_DosFsVolDescGet

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct DOS_VOLUME_DESC* DOS_VOLUME_DESC_ID;

    STATUS dosFsVolFormat(char* path, int opt, FUNCPTR pPromptFunc);
    DOS_VOLUME_DESC_ID dosFsVolDescGet(void* pDevNameOrPVolDesc, u_char** ppTail);

#ifdef __cplusplus
}
#endif

#endif /* DOSFSLIB_H */
