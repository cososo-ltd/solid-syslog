/* A test stand-in for the VxWorks 6.4 semaphore library header.
 *
 * Supplies the subset of semLib.h that Platform/VxWorks64 calls, with the
 * prototypes of the public API reference. The semaphore itself is opaque to
 * callers, so the struct is declared and never defined.
 *
 * It does not include vxWorks.h. VxWorks expects that header first in every
 * translation unit, and leaving it out here makes a source that forgets it
 * fail on the host rather than only against the real headers.
 */
#ifndef SEMLIB_H
#define SEMLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

/* #defines because they are #defines in the real header. The suppression
 * lives here rather than in a .clang-tidy because clang-tidy resolves its
 * config from the translation unit, and both the pack's sources and the tests
 * include this. */
/* NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */
#define SEM_Q_PRIORITY 0x1
#define SEM_DELETE_SAFE 0x4
#define SEM_INVERSION_SAFE 0x8
    /* NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */

    typedef struct semaphore* SEM_ID;

    SEM_ID semMCreate(int options);
    STATUS semTake(SEM_ID semId, int timeout);
    STATUS semGive(SEM_ID semId);
    STATUS semDelete(SEM_ID semId);

#ifdef __cplusplus
}
#endif

#endif /* SEMLIB_H */
