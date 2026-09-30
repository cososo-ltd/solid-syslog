/* A test stand-in for the VxWorks 6.4 host table library header.
 *
 * Supplies the subset of hostLib.h that Platform/VxWorks64 calls, with the
 * prototype of the public API reference: the name is not const-qualified.
 */
#ifndef HOSTLIB_H
#define HOSTLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

    int hostGetByName(char* name);

#ifdef __cplusplus
}
#endif

#endif /* HOSTLIB_H */
