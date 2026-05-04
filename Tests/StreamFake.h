#ifndef STREAMFAKE_H
#define STREAMFAKE_H

#include <stddef.h>
#include <stdbool.h>

#include "ExternC.h"
#include "SolidSyslogStream.h"

EXTERN_C_BEGIN

    struct SolidSyslogStream;

    struct SolidSyslogAddress;

    struct SolidSyslogStream*        StreamFake_Create(void);
    void                             StreamFake_Destroy(struct SolidSyslogStream * stream);
    int                              StreamFake_OpenCallCount(struct SolidSyslogStream * stream);
    const struct SolidSyslogAddress* StreamFake_LastOpenAddr(struct SolidSyslogStream * stream);
    int                              StreamFake_SendCallCount(struct SolidSyslogStream * stream);
    const void*                      StreamFake_LastSendBuf(struct SolidSyslogStream * stream);
    size_t                           StreamFake_LastSendSize(struct SolidSyslogStream * stream);
    int                              StreamFake_ReadCallCount(struct SolidSyslogStream * stream);
    void*                            StreamFake_LastReadBuf(struct SolidSyslogStream * stream);
    size_t                           StreamFake_LastReadSize(struct SolidSyslogStream * stream);
    void                             StreamFake_SetReadReturn(struct SolidSyslogStream * stream, SolidSyslogSsize value);
    void                             StreamFake_SetOpenFails(struct SolidSyslogStream * stream, bool fails);
    int                              StreamFake_CloseCallCount(struct SolidSyslogStream * stream);

EXTERN_C_END

#endif /* STREAMFAKE_H */
