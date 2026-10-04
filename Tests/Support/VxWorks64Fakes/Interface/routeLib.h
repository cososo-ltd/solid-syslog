/* A test stand-in for the VxWorks 6.4 routing library header.
 *
 * Supplies the subset of routeLib.h that the BDD target calls, with the
 * prototype of the public API reference: neither address is const-qualified.
 */
#ifndef ROUTELIB_H
#define ROUTELIB_H

#ifdef __cplusplus
extern "C"
{
#endif

    STATUS routeAdd(char* destination, char* gateway);

#ifdef __cplusplus
}
#endif

#endif /* ROUTELIB_H */
