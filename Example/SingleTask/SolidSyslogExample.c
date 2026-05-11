#include "SolidSyslogExample.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "ExampleAppName.h"
#include "ExampleCommandLine.h"
#include "ExampleEnterpriseId.h"
#include "ExampleInteractive.h"
#include "ExampleIps.h"
#include "ExampleLanguage.h"
#include "ExampleStderrErrorHandler.h"
#include "ExampleTcpConfig.h"
#include "ExampleUdpConfig.h"
#include "SolidSyslog.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogGetAddrInfoResolver.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogPosixClock.h"
#include "SolidSyslogPosixDatagram.h"
#include "SolidSyslogPosixHostname.h"
#include "SolidSyslogPosixProcessId.h"
#include "SolidSyslogPosixSysUpTime.h"
#include "SolidSyslogPosixTcpStream.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogTimeQuality.h"

static SolidSyslogPosixTcpStreamStorage tcpStreamStorage;
static SolidSyslogStreamSenderStorage   tcpSenderStorage;

static void GetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    timeQuality->tzKnown                  = true;
    timeQuality->isSynced                 = true;
    timeQuality->syncAccuracyMicroseconds = SOLIDSYSLOG_SYNC_ACCURACY_OMIT;
}

int SolidSyslogExample_Run(int argc, char* argv[])
{
    ExampleStderrErrorHandler_Install();
    ExampleAppName_Set(argv[0]);

    struct ExampleOptions options;
    if (ExampleCommandLine_Parse(argc, argv, &options) != 0)
    {
        return 1;
    }

    struct SolidSyslogResolver* resolver = SolidSyslogGetAddrInfoResolver_Create();
    struct SolidSyslogDatagram* datagram = NULL;
    struct SolidSyslogStream*   stream   = NULL;
    struct SolidSyslogSender*   sender   = NULL;
    bool                        useTcp   = (options.transport != NULL) && (strcmp(options.transport, "tcp") == 0);

    if (useTcp)
    {
        stream                                         = SolidSyslogPosixTcpStream_Create(&tcpStreamStorage);
        struct SolidSyslogStreamSenderConfig tcpConfig = {
            .resolver        = resolver,
            .stream          = stream,
            .endpoint        = ExampleTcpConfig_GetEndpoint,
            .endpointVersion = ExampleTcpConfig_GetEndpointVersion,
        };
        sender = SolidSyslogStreamSender_Create(&tcpSenderStorage, &tcpConfig);
    }
    else
    {
        datagram                                    = SolidSyslogPosixDatagram_Create();
        struct SolidSyslogUdpSenderConfig udpConfig = {
            .resolver        = resolver,
            .datagram        = datagram,
            .endpoint        = ExampleUdpConfig_GetEndpoint,
            .endpointVersion = ExampleUdpConfig_GetEndpointVersion,
        };
        sender = SolidSyslogUdpSender_Create(&udpConfig);
    }
    struct SolidSyslogBuffer*        buffer     = SolidSyslogNullBuffer_Create(sender);
    struct SolidSyslogStore*         store      = SolidSyslogNullStore_Create();
    struct SolidSyslogAtomicCounter* counter    = SolidSyslogAtomicCounter_Create();
    struct SolidSyslogMetaSdConfig   metaConfig = {
          .counter      = counter,
          .getSysUpTime = SolidSyslogPosixSysUpTime_Get,
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
        .sender       = NULL,
        .clock        = SolidSyslogPosixClock_GetTimestamp,
        .getHostname  = SolidSyslogPosixHostname_Get,
        .getAppName   = ExampleAppName_Get,
        .getProcessId = SolidSyslogPosixProcessId_Get,
        .store        = store,
        .sd           = sdList,
        .sdCount      = sizeof(sdList) / sizeof(sdList[0]),
    };
    SolidSyslog_Create(&config);

    struct SolidSyslogMessage message = {
        .facility  = options.facility,
        .severity  = options.severity,
        .messageId = options.messageId,
        .msg       = options.msg,
    };

    ExampleInteractive_Run(&message, stdin, NULL, NULL);

    SolidSyslog_Destroy();
    SolidSyslogOriginSd_Destroy();
    SolidSyslogTimeQualitySd_Destroy();
    SolidSyslogMetaSd_Destroy();
    SolidSyslogAtomicCounter_Destroy();
    SolidSyslogNullStore_Destroy();
    SolidSyslogNullBuffer_Destroy();
    if (useTcp)
    {
        SolidSyslogStreamSender_Destroy(sender);
        SolidSyslogPosixTcpStream_Destroy(stream);
    }
    else
    {
        SolidSyslogUdpSender_Destroy();
        SolidSyslogPosixDatagram_Destroy();
    }
    SolidSyslogGetAddrInfoResolver_Destroy();

    return 0;
}
