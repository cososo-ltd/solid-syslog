/* A test stand-in for the VxWorks 6.4 host table library header.
 *
 * Supplies the subset of hostLib.h that Platform/VxWorks64 calls, with the
 * prototypes of the public API reference: no name is const-qualified, and
 * lengths are int.
 */
#ifndef HOSTLIB_H
#define HOSTLIB_H

/* NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */
#define MAXHOSTNAMELEN 256
/* NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */

#ifdef __cplusplus
extern "C"
{
#endif

    int hostGetByName(char* name);
    int gethostname(char* name, int nameLen);
    int sethostname(char* name, int nameLen);

#ifdef __cplusplus
}
#endif

#endif /* HOSTLIB_H */
