#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "TestUtils.h"

#include "SolidSyslogAddressErrors.h"
#include "SolidSyslogAesGcmPolicyErrors.h"
#include "SolidSyslogAtomicCounterErrors.h"
#include "SolidSyslogBlockStoreErrors.h"
#include "SolidSyslogCircularBufferErrors.h"
#include "SolidSyslogDatagramErrors.h"
#include "SolidSyslogErrors.h"
#include "SolidSyslogFileBlockDeviceErrors.h"
#include "SolidSyslogFileErrors.h"
#include "SolidSyslogHmacSha256PolicyErrors.h"
#include "SolidSyslogMetaSdErrors.h"
#include "SolidSyslogMutexErrors.h"
#include "SolidSyslogOriginSdErrors.h"
#include "SolidSyslogPassthroughBufferErrors.h"
#include "SolidSyslogResolverErrors.h"
#include "SolidSyslogStreamSenderErrors.h"
#include "SolidSyslogSwitchingSenderErrors.h"
#include "SolidSyslogTcpStreamErrors.h"
#include "SolidSyslogTimeQualitySdErrors.h"
#include "SolidSyslogTlsCredentialsErrors.h"
#include "SolidSyslogTlsStreamErrors.h"
#include "SolidSyslogUdpSenderErrors.h"

TEST_GROUP(SolidSyslogErrorNames){};

TEST(SolidSyslogErrorNames, AddressErrorsNamesItsCodes)
{
    STRCMP_EQUAL("POOL_EXHAUSTED", SolidSyslogAddressErrors_Name(SOLIDSYSLOG_ADDRESS_ERROR_POOL_EXHAUSTED));
    CHECK_VOCABULARY_NAMED(SolidSyslogAddressErrors_Name, SOLIDSYSLOG_ADDRESS_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, AesGcmPolicyErrorsNamesItsCodes)
{
    STRCMP_EQUAL("BAD_CONFIG", SolidSyslogAesGcmPolicyErrors_Name(SOLIDSYSLOG_AES_GCM_POLICY_ERROR_BAD_CONFIG));
    CHECK_VOCABULARY_NAMED(SolidSyslogAesGcmPolicyErrors_Name, SOLIDSYSLOG_AES_GCM_POLICY_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, AtomicCounterErrorsNamesItsCodes)
{
    STRCMP_EQUAL(
        "POOL_EXHAUSTED",
        SolidSyslogAtomicCounterErrors_Name(SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_POOL_EXHAUSTED)
    );
    CHECK_VOCABULARY_NAMED(SolidSyslogAtomicCounterErrors_Name, SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, BlockStoreErrorsNamesItsCodes)
{
    STRCMP_EQUAL("BLOCK_TOO_SMALL", SolidSyslogBlockStoreErrors_Name(SOLIDSYSLOG_BLOCK_STORE_ERROR_BLOCK_TOO_SMALL));
    CHECK_VOCABULARY_NAMED(SolidSyslogBlockStoreErrors_Name, SOLIDSYSLOG_BLOCK_STORE_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, CircularBufferErrorsNamesItsCodes)
{
    STRCMP_EQUAL(
        "RECORD_TOO_LARGE",
        SolidSyslogCircularBufferErrors_Name(SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_TOO_LARGE)
    );
    CHECK_VOCABULARY_NAMED(SolidSyslogCircularBufferErrors_Name, SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, DatagramErrorsNamesItsCodes)
{
    STRCMP_EQUAL("NEXT_HOP_UNRESOLVED", SolidSyslogDatagramErrors_Name(SOLIDSYSLOG_DATAGRAM_ERROR_NEXT_HOP_UNRESOLVED));
    CHECK_VOCABULARY_NAMED(SolidSyslogDatagramErrors_Name, SOLIDSYSLOG_DATAGRAM_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, ErrorsNamesItsCodes)
{
    STRCMP_EQUAL("CREATE_NULL_CONFIG", SolidSyslogErrors_Name(SOLIDSYSLOG_ERROR_CREATE_NULL_CONFIG));
    CHECK_VOCABULARY_NAMED(SolidSyslogErrors_Name, SOLIDSYSLOG_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, FileBlockDeviceErrorsNamesItsCodes)
{
    STRCMP_EQUAL(
        "POOL_EXHAUSTED",
        SolidSyslogFileBlockDeviceErrors_Name(SOLIDSYSLOG_FILE_BLOCK_DEVICE_ERROR_POOL_EXHAUSTED)
    );
    CHECK_VOCABULARY_NAMED(SolidSyslogFileBlockDeviceErrors_Name, SOLIDSYSLOG_FILE_BLOCK_DEVICE_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, FileErrorsNamesItsCodes)
{
    STRCMP_EQUAL("NULL_FILESYSTEM", SolidSyslogFileErrors_Name(SOLIDSYSLOG_FILE_ERROR_NULL_FILESYSTEM));
    CHECK_VOCABULARY_NAMED(SolidSyslogFileErrors_Name, SOLIDSYSLOG_FILE_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, HmacSha256PolicyErrorsNamesItsCodes)
{
    STRCMP_EQUAL("BAD_CONFIG", SolidSyslogHmacSha256PolicyErrors_Name(SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_BAD_CONFIG));
    CHECK_VOCABULARY_NAMED(SolidSyslogHmacSha256PolicyErrors_Name, SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, MetaSdErrorsNamesItsCodes)
{
    STRCMP_EQUAL("NULL_CONFIG", SolidSyslogMetaSdErrors_Name(SOLIDSYSLOG_META_SD_ERROR_NULL_CONFIG));
    CHECK_VOCABULARY_NAMED(SolidSyslogMetaSdErrors_Name, SOLIDSYSLOG_META_SD_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, MutexErrorsNamesItsCodes)
{
    STRCMP_EQUAL("CREATE_FAILED", SolidSyslogMutexErrors_Name(SOLIDSYSLOG_MUTEX_ERROR_CREATE_FAILED));
    CHECK_VOCABULARY_NAMED(SolidSyslogMutexErrors_Name, SOLIDSYSLOG_MUTEX_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, OriginSdErrorsNamesItsCodes)
{
    STRCMP_EQUAL("POOL_EXHAUSTED", SolidSyslogOriginSdErrors_Name(SOLIDSYSLOG_ORIGIN_SD_ERROR_POOL_EXHAUSTED));
    CHECK_VOCABULARY_NAMED(SolidSyslogOriginSdErrors_Name, SOLIDSYSLOG_ORIGIN_SD_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, PassthroughBufferErrorsNamesItsCodes)
{
    STRCMP_EQUAL(
        "NULL_SENDER",
        SolidSyslogPassthroughBufferErrors_Name(SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_NULL_SENDER)
    );
    CHECK_VOCABULARY_NAMED(SolidSyslogPassthroughBufferErrors_Name, SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, ResolverErrorsNamesItsCodes)
{
    STRCMP_EQUAL("RESOLVE_TIMEOUT", SolidSyslogResolverErrors_Name(SOLIDSYSLOG_RESOLVER_ERROR_RESOLVE_TIMEOUT));
    CHECK_VOCABULARY_NAMED(SolidSyslogResolverErrors_Name, SOLIDSYSLOG_RESOLVER_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, StreamSenderErrorsNamesItsCodes)
{
    STRCMP_EQUAL("NULL_CONFIG", SolidSyslogStreamSenderErrors_Name(SOLIDSYSLOG_STREAM_SENDER_ERROR_NULL_CONFIG));
    CHECK_VOCABULARY_NAMED(SolidSyslogStreamSenderErrors_Name, SOLIDSYSLOG_STREAM_SENDER_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, SwitchingSenderErrorsNamesItsCodes)
{
    STRCMP_EQUAL("NULL_CONFIG", SolidSyslogSwitchingSenderErrors_Name(SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NULL_CONFIG));
    CHECK_VOCABULARY_NAMED(SolidSyslogSwitchingSenderErrors_Name, SOLIDSYSLOG_SWITCHING_SENDER_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, TcpStreamErrorsNamesItsCodes)
{
    STRCMP_EQUAL("NULL_CONFIG", SolidSyslogTcpStreamErrors_Name(SOLIDSYSLOG_TCP_STREAM_ERROR_NULL_CONFIG));
    CHECK_VOCABULARY_NAMED(SolidSyslogTcpStreamErrors_Name, SOLIDSYSLOG_TCP_STREAM_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, TimeQualitySdErrorsNamesItsCodes)
{
    STRCMP_EQUAL("NULL_CALLBACK", SolidSyslogTimeQualitySdErrors_Name(SOLIDSYSLOG_TIME_QUALITY_SD_ERROR_NULL_CALLBACK));
    CHECK_VOCABULARY_NAMED(SolidSyslogTimeQualitySdErrors_Name, SOLIDSYSLOG_TIME_QUALITY_SD_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, TlsCredentialsErrorsNamesItsCodes)
{
    STRCMP_EQUAL("NULL_CONFIG", SolidSyslogTlsCredentialsErrors_Name(SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_NULL_CONFIG));
    CHECK_VOCABULARY_NAMED(SolidSyslogTlsCredentialsErrors_Name, SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, TlsStreamErrorsNamesItsCodes)
{
    STRCMP_EQUAL(
        "LIBRARY_OUT_OF_MEMORY",
        SolidSyslogTlsStreamErrors_Name(SOLIDSYSLOG_TLS_STREAM_ERROR_LIBRARY_OUT_OF_MEMORY)
    );
    CHECK_VOCABULARY_NAMED(SolidSyslogTlsStreamErrors_Name, SOLIDSYSLOG_TLS_STREAM_ERROR_MAX);
}

TEST(SolidSyslogErrorNames, UdpSenderErrorsNamesItsCodes)
{
    STRCMP_EQUAL("NULL_CONFIG", SolidSyslogUdpSenderErrors_Name(SOLIDSYSLOG_UDP_SENDER_ERROR_NULL_CONFIG));
    CHECK_VOCABULARY_NAMED(SolidSyslogUdpSenderErrors_Name, SOLIDSYSLOG_UDP_SENDER_ERROR_MAX);
}
