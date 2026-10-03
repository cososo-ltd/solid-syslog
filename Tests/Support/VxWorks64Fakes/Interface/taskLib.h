/* A test stand-in for the VxWorks 6.4 task library header.
 *
 * Supplies the subset of taskLib.h that the BDD target calls, with the
 * prototypes of the public API reference.
 */
#ifndef TASKLIB_H
#define TASKLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

    int taskSpawn(
        char* name,
        int priority,
        int options,
        int stackSize,
        FUNCPTR entryPt,
        int arg1,
        int arg2,
        int arg3,
        int arg4,
        int arg5,
        int arg6,
        int arg7,
        int arg8,
        int arg9,
        int arg10
    );
    STATUS taskDelay(int ticks);

#ifdef __cplusplus
}
#endif

#endif /* TASKLIB_H */
