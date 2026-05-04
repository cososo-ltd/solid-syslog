#include "SolidSyslogWindowsExample.h"
#include "ExampleAppName.h"
#include "ExampleEnterpriseId.h"
#include "ExampleInteractive.h"
#include "ExampleIps.h"
#include "ExampleLanguage.h"
#include "ExampleMtlsConfig.h"
#include "ExampleServiceThread.h"
#include "ExampleTlsConfig.h"
#include "ExampleTlsSender.h"
#include "ExampleWindowsCommandLine.h"
#include "SolidSyslog.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogWindowsAtomicOps.h"
#include "SolidSyslogWindowsClock.h"
#include "SolidSyslogWindowsHostname.h"
#include "SolidSyslogWindowsMutex.h"
#include "SolidSyslogWindowsProcessId.h"
#include "SolidSyslogWindowsSysUpTime.h"
#include "SolidSyslogWinsockDatagram.h"
#include "SolidSyslogWinsockResolver.h"
#include "SolidSyslogWinsockTcpStream.h"

#include <process.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
// windows.h must follow winsock2.h to avoid winsock1/2 declaration conflicts
#include <windows.h>

enum
{
    /* Unprivileged mirror of SOLIDSYSLOG_UDP_DEFAULT_PORT (514) /
       SOLIDSYSLOG_TCP_DEFAULT_PORT (601). The BDD oracle listens on both
       UDP and TCP at 5514. */
    EXAMPLE_PORT            = 5514,
    EXAMPLE_BUFFER_MESSAGES = 10
};

static SolidSyslogWinsockTcpStreamStorage tcpStreamStorage;
static SolidSyslogStreamSenderStorage     tcpSenderStorage;
static SolidSyslogCircularBufferStorage   bufferStorage[SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE(EXAMPLE_BUFFER_MESSAGES)];
static SolidSyslogWindowsMutexStorage     mutexStorage;
static volatile bool                      shutdownFlag;

// NOLINTNEXTLINE(readability-non-const-parameter) -- _beginthreadex thread-entry signature requires void*
static unsigned __stdcall ServiceThreadEntry(void* arg)
{
    ExampleServiceThread_Run((volatile bool*) arg);
    return 0;
}

static const char* GetHost(void)
{
    return "127.0.0.1";
}

static int GetPort(void)
{
    return EXAMPLE_PORT;
}

static void GetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->host, GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->port = (uint16_t) GetPort();
}

static uint32_t GetEndpointVersion(void)
{
    return 0;
}

static void GetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    timeQuality->tzKnown                  = true;
    timeQuality->isSynced                 = true;
    timeQuality->syncAccuracyMicroseconds = SOLIDSYSLOG_SYNC_ACCURACY_OMIT;
}

/* MSVC's getenv triggers C4996; getenv_s is the strict-mode equivalent.
   Static buffer is single-thread-safe (called once from _Run before threads
   start) and large enough for any reasonable hostname. Returns the buffer
   when the env var is set and non-empty, NULL otherwise. */
static const char* GetEnvVar(char* buffer, size_t bufferSize, const char* name)
{
    size_t  requiredSize = 0;
    errno_t err          = getenv_s(&requiredSize, buffer, bufferSize, name);
    if ((err != 0) || (requiredSize == 0))
    {
        return NULL;
    }
    return buffer;
}

int SolidSyslogWindowsExample_Run(int argc, char* argv[])
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return 1;
    }

    ExampleAppName_Set(argv[0]);

    /* BDD harness can override the TLS/mTLS host (defaults to "syslog-ng",
       the Linux compose service name). Same env-var contract as the Threaded
       example so behave can target either oracle. */
    static char tlsHostBuffer[256];
    static char mtlsHostBuffer[256];
    ExampleTlsConfig_SetHost(GetEnvVar(tlsHostBuffer, sizeof tlsHostBuffer, "SOLIDSYSLOG_BDD_TLS_HOST"));
    ExampleMtlsConfig_SetHost(GetEnvVar(mtlsHostBuffer, sizeof mtlsHostBuffer, "SOLIDSYSLOG_BDD_MTLS_HOST"));

    struct WindowsExampleOptions options;
    ExampleWindowsCommandLine_Parse(argc, argv, &options);

    struct SolidSyslogResolver* resolver = SolidSyslogWinsockResolver_Create();
    struct SolidSyslogDatagram* datagram = NULL;
    struct SolidSyslogStream*   stream   = NULL;
    struct SolidSyslogSender*   sender   = NULL;

    bool useTcp  = (strcmp(options.transport, "tcp") == 0);
    bool useTls  = (strcmp(options.transport, "tls") == 0);
    bool useMtls = (strcmp(options.transport, "mtls") == 0);

    if (useTls || useMtls)
    {
        sender = ExampleTlsSender_Create(resolver, useMtls);
    }
    else if (useTcp)
    {
        stream                                         = SolidSyslogWinsockTcpStream_Create(&tcpStreamStorage);
        struct SolidSyslogStreamSenderConfig tcpConfig = {
            .resolver = resolver, .stream = stream, .endpoint = GetEndpoint, .endpointVersion = GetEndpointVersion};
        sender = SolidSyslogStreamSender_Create(&tcpSenderStorage, &tcpConfig);
    }
    else
    {
        datagram                                    = SolidSyslogWinsockDatagram_Create();
        struct SolidSyslogUdpSenderConfig udpConfig = {
            .resolver = resolver, .datagram = datagram, .endpoint = GetEndpoint, .endpointVersion = GetEndpointVersion};
        sender = SolidSyslogUdpSender_Create(&udpConfig);
    }
    struct SolidSyslogMutex*         mutex      = SolidSyslogWindowsMutex_Create(&mutexStorage);
    struct SolidSyslogBuffer*        buffer     = SolidSyslogCircularBuffer_Create(bufferStorage, sizeof(bufferStorage), mutex);
    struct SolidSyslogStore*         store      = SolidSyslogNullStore_Create();
    struct SolidSyslogAtomicCounter* counter    = SolidSyslogAtomicCounter_Create(SolidSyslogWindowsAtomicOps_Create());
    struct SolidSyslogMetaSdConfig   metaConfig = {
          .counter      = counter,
          .getSysUpTime = SolidSyslogWindowsSysUpTime_Get,
          .getLanguage  = ExampleLanguage_Get,
    };
    struct SolidSyslogStructuredData* metaSd       = SolidSyslogMetaSd_Create(&metaConfig);
    struct SolidSyslogStructuredData* timeQuality  = SolidSyslogTimeQualitySd_Create(GetTimeQuality);
    struct SolidSyslogOriginSdConfig  originConfig = {
         .software     = "SolidSyslogExample",
         .swVersion    = "0.7.0",
         .enterpriseId = EXAMPLE_ENTERPRISE_ID,
         .getIpCount   = ExampleIps_Count,
         .getIpAt      = ExampleIps_At,
    };
    struct SolidSyslogStructuredData* originSd = SolidSyslogOriginSd_Create(&originConfig);

    struct SolidSyslogStructuredData* sdList[] = {metaSd, timeQuality, originSd};

    struct SolidSyslogConfig config = {
        .buffer       = buffer,
        .sender       = sender,
        .clock        = SolidSyslogWindowsClock_GetTimestamp,
        .getHostname  = SolidSyslogWindowsHostname_Get,
        .getAppName   = ExampleAppName_Get,
        .getProcessId = SolidSyslogWindowsProcessId_Get,
        .store        = store,
        .sd           = sdList,
        .sdCount      = sizeof(sdList) / sizeof(sdList[0]),
    };
    SolidSyslog_Create(&config);

    shutdownFlag         = false;
    HANDLE serviceThread = (HANDLE) _beginthreadex(NULL, 0, ServiceThreadEntry, (void*) &shutdownFlag, 0, NULL);

    struct SolidSyslogMessage message = {
        .facility  = options.facility,
        .severity  = options.severity,
        .messageId = options.messageId,
        .msg       = options.msg,
    };

    ExampleInteractive_Run(&message, stdin, NULL);

    shutdownFlag = true;
    WaitForSingleObject(serviceThread, INFINITE);
    CloseHandle(serviceThread);

    SolidSyslog_Destroy();
    SolidSyslogOriginSd_Destroy();
    SolidSyslogTimeQualitySd_Destroy();
    SolidSyslogMetaSd_Destroy();
    SolidSyslogAtomicCounter_Destroy();
    SolidSyslogWindowsAtomicOps_Destroy();
    SolidSyslogNullStore_Destroy();
    SolidSyslogCircularBuffer_Destroy(buffer);
    SolidSyslogWindowsMutex_Destroy(mutex);
    if (useTls || useMtls)
    {
        ExampleTlsSender_Destroy();
    }
    else if (useTcp)
    {
        SolidSyslogStreamSender_Destroy(sender);
        SolidSyslogWinsockTcpStream_Destroy(stream);
    }
    else
    {
        SolidSyslogUdpSender_Destroy();
        SolidSyslogWinsockDatagram_Destroy();
    }
    SolidSyslogWinsockResolver_Destroy();

    WSACleanup();

    return 0;
}
