#ifndef SOLIDSYSLOGFREERTOSTCPSTREAM_H
#define SOLIDSYSLOGFREERTOSTCPSTREAM_H

#include "ExternC.h"
#include "SolidSyslogTcpConnectTimeoutFunction.h"

EXTERN_C_BEGIN

    struct SolidSyslogStream;

    struct SolidSyslogFreeRtosTcpStreamConfig
    {
        SolidSyslogTcpConnectTimeoutFunction
            GetConnectTimeoutMs; /* NULL → use SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable */
        void* ConnectTimeoutContext; /* passed through to GetConnectTimeoutMs; NULL is fine */
    };

    struct SolidSyslogStream* SolidSyslogFreeRtosTcpStream_Create(
        const struct SolidSyslogFreeRtosTcpStreamConfig* config
    );
    void SolidSyslogFreeRtosTcpStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSTCPSTREAM_H */
