#ifndef PLUSFATFAKE_H
#define PLUSFATFAKE_H

#include "ExternC.h"

EXTERN_C_BEGIN

    void PlusFatFake_Reset(void);

    /* ff_fopen */
    void PlusFatFake_SetOpenFailsForMode(const char* mode);
    void PlusFatFake_SetOpenAlwaysFails(void);
    int PlusFatFake_OpenCallCount(void);
    const char* PlusFatFake_OpenModeAt(int index);
    const char* PlusFatFake_LastOpenPath(void);

    /* ff_fclose */
    int PlusFatFake_CloseCallCount(void);

    /* ff_fread */
    void PlusFatFake_SetReadSource(const void* bytes, unsigned long count);
    int PlusFatFake_ReadCallCount(void);
    unsigned long PlusFatFake_LastReadSize(void);
    unsigned long PlusFatFake_LastReadItems(void);

    /* ff_fwrite */
    void PlusFatFake_SetWriteIncomplete(void);
    int PlusFatFake_WriteCallCount(void);
    const void* PlusFatFake_LastWriteBytes(void);
    unsigned long PlusFatFake_LastWriteItems(void);

    /* FF_FlushCache */
    void PlusFatFake_SetFlushCacheFails(void);
    int PlusFatFake_FlushCacheCallCount(void);

    /* ff_fseek */
    int PlusFatFake_SeekCallCount(void);
    long PlusFatFake_LastSeekOffset(void);
    int PlusFatFake_LastSeekWhence(void);

    /* ff_filelength */
    void PlusFatFake_SetFileLength(unsigned long length);

    /* ff_seteof */
    int PlusFatFake_SeteofCallCount(void);

    /* ff_stat */
    void PlusFatFake_SetStatFails(void);
    int PlusFatFake_StatCallCount(void);
    const char* PlusFatFake_LastStatPath(void);

    /* ff_remove */
    void PlusFatFake_SetRemoveFails(void);
    int PlusFatFake_RemoveCallCount(void);
    const char* PlusFatFake_LastRemovePath(void);

EXTERN_C_END

#endif /* PLUSFATFAKE_H */
