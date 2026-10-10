/* A test stand-in for the VxWorks 6.4 errno library header.
 *
 * Supplies the subset of errnoLib.h that the BDD target calls, with the
 * prototype of the public API reference.
 */
#ifndef ERRNOLIB_H
#define ERRNOLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

    int errnoGet(void);

#ifdef __cplusplus
}
#endif

#endif /* ERRNOLIB_H */
