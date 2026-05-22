#ifndef SOLIDSYSLOGERRORMESSAGES_H
#define SOLIDSYSLOGERRORMESSAGES_H

#define SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_CONFIG "SolidSyslog_Create called with NULL config"
#define SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_BUFFER "SolidSyslog_Create config.Buffer is NULL"
#define SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_SENDER "SolidSyslog_Create config.Sender is NULL"
#define SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_STORE "SolidSyslog_Create config.Store is NULL"
#define SOLIDSYSLOG_ERROR_MSG_LOG_NULL_MESSAGE "SolidSyslog_Log called with NULL message"
#define SOLIDSYSLOG_ERROR_MSG_LOG_NULL_HANDLE "SolidSyslog_Log called with NULL handle"
#define SOLIDSYSLOG_ERROR_MSG_SERVICE_NULL_HANDLE "SolidSyslog_Service called with NULL handle"
#define SOLIDSYSLOG_ERROR_MSG_SOLIDSYSLOG_POOL_EXHAUSTED \
    "SolidSyslog_Create pool exhausted; returning fallback instance"
#define SOLIDSYSLOG_ERROR_MSG_SOLIDSYSLOG_UNKNOWN_DESTROY \
    "SolidSyslog_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_METASD_CREATE_NULL_CONFIG "SolidSyslogMetaSd_Create called with NULL config"
#define SOLIDSYSLOG_ERROR_MSG_METASD_CREATE_NULL_COUNTER "SolidSyslogMetaSd_Create config.Counter is NULL"
#define SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_CONFIG "SolidSyslogUdpSender_Create called with NULL config"
#define SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_RESOLVER "SolidSyslogUdpSender_Create config.Resolver is NULL"
#define SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_DATAGRAM "SolidSyslogUdpSender_Create config.Datagram is NULL"
#define SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_ADDRESS "SolidSyslogUdpSender_Create config.Address is NULL"
#define SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_ENDPOINT "SolidSyslogUdpSender_Create config.Endpoint is NULL"
#define SOLIDSYSLOG_ERROR_MSG_UDPSENDER_SEND_NULL_BUFFER "SolidSyslogUdpSender_Send called with NULL buffer"
#define SOLIDSYSLOG_ERROR_MSG_UDPSENDER_POOL_EXHAUSTED \
    "SolidSyslogUdpSender_Create pool exhausted; returning fallback sender"
#define SOLIDSYSLOG_ERROR_MSG_UDPSENDER_UNKNOWN_DESTROY \
    "SolidSyslogUdpSender_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_SWITCHINGSENDER_CREATE_NULL_CONFIG \
    "SolidSyslogSwitchingSender_Create called with NULL config"
#define SOLIDSYSLOG_ERROR_MSG_SWITCHINGSENDER_CREATE_NULL_SENDERS \
    "SolidSyslogSwitchingSender_Create config.Senders is NULL"
#define SOLIDSYSLOG_ERROR_MSG_SWITCHINGSENDER_CREATE_NULL_SELECTOR \
    "SolidSyslogSwitchingSender_Create config.Selector is NULL"
#define SOLIDSYSLOG_ERROR_MSG_SWITCHINGSENDER_POOL_EXHAUSTED \
    "SolidSyslogSwitchingSender_Create pool exhausted; returning fallback sender"
#define SOLIDSYSLOG_ERROR_MSG_SWITCHINGSENDER_UNKNOWN_DESTROY \
    "SolidSyslogSwitchingSender_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_METASD_POOL_EXHAUSTED "SolidSyslogMetaSd_Create pool exhausted; returning fallback SD"
#define SOLIDSYSLOG_ERROR_MSG_METASD_UNKNOWN_DESTROY \
    "SolidSyslogMetaSd_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_TIMEQUALITYSD_CREATE_NULL_CALLBACK \
    "SolidSyslogTimeQualitySd_Create called with NULL getTimeQuality"
#define SOLIDSYSLOG_ERROR_MSG_TIMEQUALITYSD_POOL_EXHAUSTED \
    "SolidSyslogTimeQualitySd_Create pool exhausted; returning fallback SD"
#define SOLIDSYSLOG_ERROR_MSG_TIMEQUALITYSD_UNKNOWN_DESTROY \
    "SolidSyslogTimeQualitySd_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_ORIGINSD_POOL_EXHAUSTED "SolidSyslogOriginSd_Create pool exhausted; returning fallback SD"
#define SOLIDSYSLOG_ERROR_MSG_ORIGINSD_UNKNOWN_DESTROY \
    "SolidSyslogOriginSd_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_CIRCULARBUFFER_POOL_EXHAUSTED \
    "SolidSyslogCircularBuffer_Create pool exhausted; returning fallback buffer"
#define SOLIDSYSLOG_ERROR_MSG_CIRCULARBUFFER_UNKNOWN_DESTROY \
    "SolidSyslogCircularBuffer_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_PASSTHROUGHBUFFER_POOL_EXHAUSTED \
    "SolidSyslogPassthroughBuffer_Create pool exhausted; returning fallback buffer"
#define SOLIDSYSLOG_ERROR_MSG_PASSTHROUGHBUFFER_UNKNOWN_DESTROY \
    "SolidSyslogPassthroughBuffer_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_STREAMSENDER_POOL_EXHAUSTED \
    "SolidSyslogStreamSender_Create pool exhausted; returning fallback sender"
#define SOLIDSYSLOG_ERROR_MSG_STREAMSENDER_UNKNOWN_DESTROY \
    "SolidSyslogStreamSender_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_FILEBLOCKDEVICE_POOL_EXHAUSTED \
    "SolidSyslogFileBlockDevice_Create pool exhausted; returning fallback block device"
#define SOLIDSYSLOG_ERROR_MSG_FILEBLOCKDEVICE_UNKNOWN_DESTROY \
    "SolidSyslogFileBlockDevice_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_BLOCKSTORE_POOL_EXHAUSTED \
    "SolidSyslogBlockStore_Create pool exhausted; returning fallback store"
#define SOLIDSYSLOG_ERROR_MSG_BLOCKSTORE_UNKNOWN_DESTROY \
    "SolidSyslogBlockStore_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_POSIXMUTEX_POOL_EXHAUSTED \
    "SolidSyslogPosixMutex_Create pool exhausted; returning fallback mutex"
#define SOLIDSYSLOG_ERROR_MSG_POSIXMUTEX_UNKNOWN_DESTROY \
    "SolidSyslogPosixMutex_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_POSIXADDRESS_POOL_EXHAUSTED \
    "SolidSyslogPosixAddress_Create pool exhausted; returning fallback address"
#define SOLIDSYSLOG_ERROR_MSG_POSIXADDRESS_UNKNOWN_DESTROY \
    "SolidSyslogPosixAddress_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_POSIXDATAGRAM_POOL_EXHAUSTED \
    "SolidSyslogPosixDatagram_Create pool exhausted; returning fallback datagram"
#define SOLIDSYSLOG_ERROR_MSG_POSIXDATAGRAM_UNKNOWN_DESTROY \
    "SolidSyslogPosixDatagram_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_GETADDRINFORESOLVER_POOL_EXHAUSTED \
    "SolidSyslogGetAddrInfoResolver_Create pool exhausted; returning fallback resolver"
#define SOLIDSYSLOG_ERROR_MSG_GETADDRINFORESOLVER_UNKNOWN_DESTROY \
    "SolidSyslogGetAddrInfoResolver_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_POSIXFILE_POOL_EXHAUSTED \
    "SolidSyslogPosixFile_Create pool exhausted; returning fallback file"
#define SOLIDSYSLOG_ERROR_MSG_POSIXFILE_UNKNOWN_DESTROY \
    "SolidSyslogPosixFile_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_POSIXTCPSTREAM_POOL_EXHAUSTED \
    "SolidSyslogPosixTcpStream_Create pool exhausted; returning fallback stream"
#define SOLIDSYSLOG_ERROR_MSG_POSIXTCPSTREAM_UNKNOWN_DESTROY \
    "SolidSyslogPosixTcpStream_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_POSIXMESSAGEQUEUEBUFFER_POOL_EXHAUSTED \
    "SolidSyslogPosixMessageQueueBuffer_Create pool exhausted; returning fallback buffer"
#define SOLIDSYSLOG_ERROR_MSG_POSIXMESSAGEQUEUEBUFFER_UNKNOWN_DESTROY \
    "SolidSyslogPosixMessageQueueBuffer_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_WINDOWSMUTEX_POOL_EXHAUSTED \
    "SolidSyslogWindowsMutex_Create pool exhausted; returning fallback mutex"
#define SOLIDSYSLOG_ERROR_MSG_WINDOWSMUTEX_UNKNOWN_DESTROY \
    "SolidSyslogWindowsMutex_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_WINSOCKADDRESS_POOL_EXHAUSTED \
    "SolidSyslogWinsockAddress_Create pool exhausted; returning fallback address"
#define SOLIDSYSLOG_ERROR_MSG_WINSOCKADDRESS_UNKNOWN_DESTROY \
    "SolidSyslogWinsockAddress_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_WINSOCKDATAGRAM_POOL_EXHAUSTED \
    "SolidSyslogWinsockDatagram_Create pool exhausted; returning fallback datagram"
#define SOLIDSYSLOG_ERROR_MSG_WINSOCKDATAGRAM_UNKNOWN_DESTROY \
    "SolidSyslogWinsockDatagram_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_WINSOCKRESOLVER_POOL_EXHAUSTED \
    "SolidSyslogWinsockResolver_Create pool exhausted; returning fallback resolver"
#define SOLIDSYSLOG_ERROR_MSG_WINSOCKRESOLVER_UNKNOWN_DESTROY \
    "SolidSyslogWinsockResolver_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_WINDOWSFILE_POOL_EXHAUSTED \
    "SolidSyslogWindowsFile_Create pool exhausted; returning fallback file"
#define SOLIDSYSLOG_ERROR_MSG_WINDOWSFILE_UNKNOWN_DESTROY \
    "SolidSyslogWindowsFile_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_WINSOCKTCPSTREAM_POOL_EXHAUSTED \
    "SolidSyslogWinsockTcpStream_Create pool exhausted; returning fallback stream"
#define SOLIDSYSLOG_ERROR_MSG_WINSOCKTCPSTREAM_UNKNOWN_DESTROY \
    "SolidSyslogWinsockTcpStream_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_FREERTOSMUTEX_POOL_EXHAUSTED \
    "SolidSyslogFreeRtosMutex_Create pool exhausted; returning fallback mutex"
#define SOLIDSYSLOG_ERROR_MSG_FREERTOSMUTEX_UNKNOWN_DESTROY \
    "SolidSyslogFreeRtosMutex_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_FREERTOSADDRESS_POOL_EXHAUSTED \
    "SolidSyslogFreeRtosAddress_Create pool exhausted; returning fallback address"
#define SOLIDSYSLOG_ERROR_MSG_FREERTOSADDRESS_UNKNOWN_DESTROY \
    "SolidSyslogFreeRtosAddress_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_FREERTOSDATAGRAM_POOL_EXHAUSTED \
    "SolidSyslogFreeRtosDatagram_Create pool exhausted; returning fallback datagram"
#define SOLIDSYSLOG_ERROR_MSG_FREERTOSDATAGRAM_UNKNOWN_DESTROY \
    "SolidSyslogFreeRtosDatagram_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_FREERTOSSTATICRESOLVER_POOL_EXHAUSTED \
    "SolidSyslogFreeRtosStaticResolver_Create pool exhausted; returning fallback resolver"
#define SOLIDSYSLOG_ERROR_MSG_FREERTOSSTATICRESOLVER_UNKNOWN_DESTROY \
    "SolidSyslogFreeRtosStaticResolver_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_FREERTOSTCPSTREAM_POOL_EXHAUSTED \
    "SolidSyslogFreeRtosTcpStream_Create pool exhausted; returning fallback stream"
#define SOLIDSYSLOG_ERROR_MSG_FREERTOSTCPSTREAM_UNKNOWN_DESTROY \
    "SolidSyslogFreeRtosTcpStream_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_STDATOMICCOUNTER_POOL_EXHAUSTED \
    "SolidSyslogStdAtomicCounter_Create pool exhausted; returning fallback counter"
#define SOLIDSYSLOG_ERROR_MSG_STDATOMICCOUNTER_UNKNOWN_DESTROY \
    "SolidSyslogStdAtomicCounter_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_WINDOWSATOMICCOUNTER_POOL_EXHAUSTED \
    "SolidSyslogWindowsAtomicCounter_Create pool exhausted; returning fallback counter"
#define SOLIDSYSLOG_ERROR_MSG_WINDOWSATOMICCOUNTER_UNKNOWN_DESTROY \
    "SolidSyslogWindowsAtomicCounter_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_FATFSFILE_POOL_EXHAUSTED \
    "SolidSyslogFatFsFile_Create pool exhausted; returning fallback file"
#define SOLIDSYSLOG_ERROR_MSG_FATFSFILE_UNKNOWN_DESTROY \
    "SolidSyslogFatFsFile_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_TLSSTREAM_POOL_EXHAUSTED \
    "SolidSyslogTlsStream_Create pool exhausted; returning fallback stream"
#define SOLIDSYSLOG_ERROR_MSG_TLSSTREAM_UNKNOWN_DESTROY \
    "SolidSyslogTlsStream_Destroy called with a handle not issued by this pool"
#define SOLIDSYSLOG_ERROR_MSG_MBEDTLSSTREAM_POOL_EXHAUSTED \
    "SolidSyslogMbedTlsStream_Create pool exhausted; returning fallback stream"
#define SOLIDSYSLOG_ERROR_MSG_MBEDTLSSTREAM_UNKNOWN_DESTROY \
    "SolidSyslogMbedTlsStream_Destroy called with a handle not issued by this pool"

#endif /* SOLIDSYSLOGERRORMESSAGES_H */
