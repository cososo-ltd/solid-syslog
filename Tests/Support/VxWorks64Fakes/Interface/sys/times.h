/* A test stand-in for the VxWorks 6.4 sys/times.h.
 *
 * Supplies the time interval connectWithTimeout bounds a connect by, with the
 * members the VxWorks header gives it. It shadows the host's header of the same
 * name, so only C sources that call the VxWorks socket API include it; a C++
 * test reads the interval through the fake's accessors instead, because a host
 * header can declare struct timeval too.
 */
#ifndef VXWORKS64FAKES_SYS_TIMES_H
#define VXWORKS64FAKES_SYS_TIMES_H

#ifdef __cplusplus
extern "C"
{
#endif

    struct timeval
    {
        long tv_sec;
        long tv_usec;
    };

#ifdef __cplusplus
}
#endif

#endif /* VXWORKS64FAKES_SYS_TIMES_H */
