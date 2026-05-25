#ifndef SOLIDSYSLOGPLUSTCPTCPSTREAM_H
#define SOLIDSYSLOGPLUSTCPTCPSTREAM_H

#include "ExternC.h"
#include "SolidSyslogTcpConnectTimeoutFunction.h"

EXTERN_C_BEGIN

    struct SolidSyslogStream;

    struct SolidSyslogPlusTcpTcpStreamConfig
    {
        SolidSyslogTcpConnectTimeoutFunction
            GetConnectTimeoutMs; /* NULL → use SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable */
        void* ConnectTimeoutContext; /* passed through to GetConnectTimeoutMs; NULL is fine */
    };

    struct SolidSyslogStream* SolidSyslogPlusTcpTcpStream_Create(const struct SolidSyslogPlusTcpTcpStreamConfig* config
    );
    void SolidSyslogPlusTcpTcpStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPTCPSTREAM_H */
