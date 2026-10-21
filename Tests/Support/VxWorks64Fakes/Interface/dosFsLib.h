/* A test stand-in for the VxWorks 6.4 dosFs library header.
 *
 * Supplies the subset of dosFsLib.h that Platform/VxWorks64 and the VxWorks 6.4
 * BDD target use: the calls renamed to the file-system fake's, and the errno
 * values as the real header defines them. Relies on vxWorks.h having been
 * included first, as VxWorks does.
 */
#ifndef DOSFSLIB_H
#define DOSFSLIB_H

#define dosFsVolFormat VxWorks64FsFake_DosFsVolFormat
#define dosFsVolDescGet VxWorks64FsFake_DosFsVolDescGet

/* NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */
#define M_dosFsLib (56 << 16)
#define S_dosFsLib_FILE_NOT_FOUND (M_dosFsLib | 3)
/* NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */

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
