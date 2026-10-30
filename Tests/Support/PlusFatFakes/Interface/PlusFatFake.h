#ifndef PLUSFATFAKE_H
#define PLUSFATFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void PlusFatFake_Reset(void);

    void PlusFatFake_SetOpenFailsForMode(const char* mode);
    void PlusFatFake_SetOpenAlwaysFails(void);
    void PlusFatFake_SetOpenErrno(int errnoValue);
    int PlusFatFake_OpenCallCount(void);
    const char* PlusFatFake_OpenModeAt(int index);
    const char* PlusFatFake_LastOpenPath(void);

    /* Makes ff_fclose fail, leaving @p errnoValue in the task errno. */
    void PlusFatFake_SetCloseErrno(int errnoValue);
    int PlusFatFake_CloseCallCount(void);

    void PlusFatFake_SetReadSource(const void* bytes, unsigned long count);
    /* Makes ff_fread deliver nothing, leaving @p errnoValue in the task errno,
     * as a read that failed does; a read that merely reaches the end leaves
     * none. */
    void PlusFatFake_SetReadErrno(int errnoValue);
    int PlusFatFake_ReadCallCount(void);
    unsigned long PlusFatFake_LastReadSize(void);
    unsigned long PlusFatFake_LastReadItems(void);

    void PlusFatFake_SetWriteIncomplete(void);
    /* Makes ff_fwrite write nothing, leaving @p errnoValue in the task errno,
     * as a write that failed does; an incomplete write leaves none. */
    void PlusFatFake_SetWriteErrno(int errnoValue);
    int PlusFatFake_WriteCallCount(void);
    const void* PlusFatFake_LastWriteBytes(void);
    unsigned long PlusFatFake_LastWriteItems(void);

    void PlusFatFake_SetFlushCacheFails(void);
    /* The FF_Error_t the failing FF_FlushCache answers. */
    int PlusFatFake_FlushCacheFailureCode(void);
    int PlusFatFake_FlushCacheCallCount(void);

    /* Makes ff_fseek fail, leaving @p errnoValue in the task errno. */
    void PlusFatFake_SetSeekErrno(int errnoValue);
    int PlusFatFake_SeekCallCount(void);
    long PlusFatFake_LastSeekOffset(void);
    int PlusFatFake_LastSeekWhence(void);

    void PlusFatFake_SetFileLength(unsigned long length);
    /* Makes ff_filelength fail, leaving @p errnoValue in the task errno. */
    void PlusFatFake_SetFileLengthErrno(int errnoValue);

    /* Makes ff_seteof fail, leaving @p errnoValue in the task errno. */
    void PlusFatFake_SetSeteofErrno(int errnoValue);
    int PlusFatFake_SeteofCallCount(void);

    void PlusFatFake_SetStatFails(void);
    int PlusFatFake_StatCallCount(void);
    const char* PlusFatFake_LastStatPath(void);

    void PlusFatFake_SetRemoveErrno(int errnoValue);
    int PlusFatFake_RemoveCallCount(void);
    const char* PlusFatFake_LastRemovePath(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* PLUSFATFAKE_H */
