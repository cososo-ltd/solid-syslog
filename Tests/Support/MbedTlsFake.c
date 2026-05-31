#include "MbedTlsFake.h"

#include <mbedtls/md.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/ssl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "mbedtls/pk.h"
#include "mbedtls/x509_crl.h"
#include "mbedtls/x509_crt.h"

enum
{
    MBEDTLSFAKE_MAX_KEY = 64,
    MBEDTLSFAKE_MAX_INPUT = 256
};

/* mbedtls_md_info_from_type / mbedtls_md_hmac */
static int mdHmacCallCount;
static int lastMdInfoType;
static uint8_t lastMdHmacKey[MBEDTLSFAKE_MAX_KEY];
static size_t lastMdHmacKeyLen;
static uint8_t lastMdHmacInput[MBEDTLSFAKE_MAX_INPUT];
static size_t lastMdHmacInputLen;
static int mdHmacReturn;

/* mbedtls_platform_zeroize */
static int platformZeroizeCallCount;
static const void* lastPlatformZeroizeBuf;
static size_t lastPlatformZeroizeLen;

/* -------------------------------------------------------------------------
 * Captured state — one section per mbedTLS API call. Tests read these via
 * accessors below; production reaches libmbedtls through the link-interposed
 * functions at the bottom of the file.
 * ------------------------------------------------------------------------- */

/* mbedtls_ssl_config_init */
static int sslConfigInitCallCount;
static mbedtls_ssl_config* lastSslConfigInitArg;

/* mbedtls_ssl_config_defaults */
static int sslConfigDefaultsCallCount;
static mbedtls_ssl_config* lastSslConfigDefaultsConfigArg;
static int lastSslConfigDefaultsEndpoint;
static int lastSslConfigDefaultsTransport;
static int lastSslConfigDefaultsPreset;
static int sslConfigDefaultsReturn;

/* mbedtls_ssl_init */
static int sslInitCallCount;
static mbedtls_ssl_context* lastSslInitArg;

/* mbedtls_ssl_setup */
static int sslSetupCallCount;
static mbedtls_ssl_context* lastSslSetupContextArg;
static const mbedtls_ssl_config* lastSslSetupConfigArg;
static int sslSetupReturn;

/* mbedtls_ssl_set_bio */
static int sslSetBioCallCount;
static mbedtls_ssl_context* lastSslSetBioContextArg;
static void* lastSslSetBioPBioArg;
static mbedtls_ssl_send_t* lastSslSetBioSendCallback;
static mbedtls_ssl_recv_t* lastSslSetBioRecvCallback;
static mbedtls_ssl_recv_timeout_t* lastSslSetBioRecvTimeoutCallback;

/* mbedtls_ssl_handshake */
enum
{
    MBEDTLSFAKE_MAX_HANDSHAKE_RETURNS = 8
};

static int sslHandshakeCallCount;
static mbedtls_ssl_context* lastSslHandshakeArg;
static int sslHandshakeReturn;
static int sslHandshakeReturnSequence[MBEDTLSFAKE_MAX_HANDSHAKE_RETURNS];
static int sslHandshakeReturnSequenceLen;

/* mbedtls_ssl_write */
static int sslWriteCallCount;
static mbedtls_ssl_context* lastSslWriteContextArg;
static const unsigned char* lastSslWriteBufArg;
static size_t lastSslWriteLenArg;
static int sslWriteReturn;
static bool sslWriteReturnSet;

/* mbedtls_ssl_read */
static int sslReadCallCount;
static mbedtls_ssl_context* lastSslReadContextArg;
static unsigned char* lastSslReadBufArg;
static size_t lastSslReadLenArg;
static int sslReadReturn;

/* mbedtls_ssl_close_notify */
static int sslCloseNotifyCallCount;
static mbedtls_ssl_context* lastSslCloseNotifyArg;

/* mbedtls_ssl_free */
static int sslFreeCallCount;
static mbedtls_ssl_context* lastSslFreeArg;

/* mbedtls_ssl_config_free */
static int sslConfigFreeCallCount;
static mbedtls_ssl_config* lastSslConfigFreeArg;

/* mbedtls_ssl_conf_authmode */
static int sslConfAuthmodeCallCount;
static mbedtls_ssl_config* lastSslConfAuthmodeConfigArg;
static int lastSslConfAuthmodeArg;

/* mbedtls_ssl_conf_ca_chain */
static int sslConfCaChainCallCount;
static mbedtls_ssl_config* lastSslConfCaChainConfigArg;
static mbedtls_x509_crt* lastSslConfCaChainArg;
static mbedtls_x509_crl* lastSslConfCaChainCrlArg;

/* mbedtls_ssl_conf_rng */
static int sslConfRngCallCount;
static mbedtls_ssl_config* lastSslConfRngConfigArg;
static int (*lastSslConfRngFuncArg)(void*, unsigned char*, size_t);
static void* lastSslConfRngContextArg;

/* mbedtls_ssl_set_hostname */
static int sslSetHostnameCallCount;
static mbedtls_ssl_context* lastSslSetHostnameContextArg;
static const char* lastSslSetHostnameNameArg;
static int sslSetHostnameReturn;

/* mbedtls_ssl_conf_own_cert */
static int sslConfOwnCertCallCount;
static mbedtls_ssl_config* lastSslConfOwnCertConfigArg;
static mbedtls_x509_crt* lastSslConfOwnCertCertArg;
static mbedtls_pk_context* lastSslConfOwnCertKeyArg;

/* -------------------------------------------------------------------------
 * Test accessors.
 * ------------------------------------------------------------------------- */

void MbedTlsFake_Reset(void)
{
    sslConfigInitCallCount = 0;
    lastSslConfigInitArg = NULL;
    sslConfigDefaultsCallCount = 0;
    lastSslConfigDefaultsConfigArg = NULL;
    lastSslConfigDefaultsEndpoint = 0;
    lastSslConfigDefaultsTransport = 0;
    lastSslConfigDefaultsPreset = 0;
    sslConfigDefaultsReturn = 0;
    sslInitCallCount = 0;
    lastSslInitArg = NULL;
    sslSetupCallCount = 0;
    lastSslSetupContextArg = NULL;
    lastSslSetupConfigArg = NULL;
    sslSetupReturn = 0;
    sslSetBioCallCount = 0;
    lastSslSetBioContextArg = NULL;
    lastSslSetBioPBioArg = NULL;
    lastSslSetBioSendCallback = NULL;
    lastSslSetBioRecvCallback = NULL;
    lastSslSetBioRecvTimeoutCallback = NULL;
    sslHandshakeCallCount = 0;
    lastSslHandshakeArg = NULL;
    sslHandshakeReturn = 0;
    sslHandshakeReturnSequenceLen = 0;
    sslWriteCallCount = 0;
    lastSslWriteContextArg = NULL;
    lastSslWriteBufArg = NULL;
    lastSslWriteLenArg = 0;
    sslWriteReturn = 0;
    sslWriteReturnSet = false;
    sslReadCallCount = 0;
    lastSslReadContextArg = NULL;
    lastSslReadBufArg = NULL;
    lastSslReadLenArg = 0;
    sslReadReturn = 0;
    sslCloseNotifyCallCount = 0;
    lastSslCloseNotifyArg = NULL;
    sslFreeCallCount = 0;
    lastSslFreeArg = NULL;
    sslConfigFreeCallCount = 0;
    lastSslConfigFreeArg = NULL;
    sslConfAuthmodeCallCount = 0;
    lastSslConfAuthmodeConfigArg = NULL;
    lastSslConfAuthmodeArg = 0;
    sslConfCaChainCallCount = 0;
    lastSslConfCaChainConfigArg = NULL;
    lastSslConfCaChainArg = NULL;
    lastSslConfCaChainCrlArg = NULL;
    sslConfRngCallCount = 0;
    lastSslConfRngConfigArg = NULL;
    lastSslConfRngFuncArg = NULL;
    lastSslConfRngContextArg = NULL;
    sslSetHostnameCallCount = 0;
    lastSslSetHostnameContextArg = NULL;
    lastSslSetHostnameNameArg = NULL;
    sslSetHostnameReturn = 0;
    sslConfOwnCertCallCount = 0;
    lastSslConfOwnCertConfigArg = NULL;
    lastSslConfOwnCertCertArg = NULL;
    lastSslConfOwnCertKeyArg = NULL;
    mdHmacCallCount = 0;
    lastMdInfoType = 0;
    lastMdHmacKeyLen = 0;
    lastMdHmacInputLen = 0;
    mdHmacReturn = 0;
    memset(lastMdHmacKey, 0, sizeof lastMdHmacKey);
    memset(lastMdHmacInput, 0, sizeof lastMdHmacInput);
    platformZeroizeCallCount = 0;
    lastPlatformZeroizeBuf = NULL;
    lastPlatformZeroizeLen = 0;
}

int MbedTlsFake_SslConfigInitCallCount(void)
{
    return sslConfigInitCallCount;
}

mbedtls_ssl_config* MbedTlsFake_LastSslConfigInitArg(void)
{
    return lastSslConfigInitArg;
}

int MbedTlsFake_SslConfigDefaultsCallCount(void)
{
    return sslConfigDefaultsCallCount;
}

mbedtls_ssl_config* MbedTlsFake_LastSslConfigDefaultsConfigArg(void)
{
    return lastSslConfigDefaultsConfigArg;
}

int MbedTlsFake_LastSslConfigDefaultsEndpoint(void)
{
    return lastSslConfigDefaultsEndpoint;
}

int MbedTlsFake_LastSslConfigDefaultsTransport(void)
{
    return lastSslConfigDefaultsTransport;
}

int MbedTlsFake_LastSslConfigDefaultsPreset(void)
{
    return lastSslConfigDefaultsPreset;
}

void MbedTlsFake_SetSslConfigDefaultsReturn(int value)
{
    sslConfigDefaultsReturn = value;
}

int MbedTlsFake_SslInitCallCount(void)
{
    return sslInitCallCount;
}

mbedtls_ssl_context* MbedTlsFake_LastSslInitArg(void)
{
    return lastSslInitArg;
}

int MbedTlsFake_SslSetupCallCount(void)
{
    return sslSetupCallCount;
}

mbedtls_ssl_context* MbedTlsFake_LastSslSetupContextArg(void)
{
    return lastSslSetupContextArg;
}

const mbedtls_ssl_config* MbedTlsFake_LastSslSetupConfigArg(void)
{
    return lastSslSetupConfigArg;
}

void MbedTlsFake_SetSslSetupReturn(int value)
{
    sslSetupReturn = value;
}

int MbedTlsFake_SslSetBioCallCount(void)
{
    return sslSetBioCallCount;
}

mbedtls_ssl_context* MbedTlsFake_LastSslSetBioContextArg(void)
{
    return lastSslSetBioContextArg;
}

void* MbedTlsFake_LastSslSetBioPBioArg(void)
{
    return lastSslSetBioPBioArg;
}

mbedtls_ssl_send_t* MbedTlsFake_LastSslSetBioSendCallback(void)
{
    return lastSslSetBioSendCallback;
}

mbedtls_ssl_recv_t* MbedTlsFake_LastSslSetBioRecvCallback(void)
{
    return lastSslSetBioRecvCallback;
}

mbedtls_ssl_recv_timeout_t* MbedTlsFake_LastSslSetBioRecvTimeoutCallback(void)
{
    return lastSslSetBioRecvTimeoutCallback;
}

int MbedTlsFake_SslHandshakeCallCount(void)
{
    return sslHandshakeCallCount;
}

mbedtls_ssl_context* MbedTlsFake_LastSslHandshakeArg(void)
{
    return lastSslHandshakeArg;
}

void MbedTlsFake_SetSslHandshakeReturn(int value)
{
    sslHandshakeReturn = value;
}

void MbedTlsFake_SetSslHandshakeReturnSequence(const int* values, int count)
{
    int safe = (count < MBEDTLSFAKE_MAX_HANDSHAKE_RETURNS) ? count : MBEDTLSFAKE_MAX_HANDSHAKE_RETURNS;
    for (int i = 0; i < safe; i++)
    {
        sslHandshakeReturnSequence[i] = values[i];
    }
    sslHandshakeReturnSequenceLen = safe;
}

int MbedTlsFake_SslWriteCallCount(void)
{
    return sslWriteCallCount;
}

mbedtls_ssl_context* MbedTlsFake_LastSslWriteContextArg(void)
{
    return lastSslWriteContextArg;
}

const unsigned char* MbedTlsFake_LastSslWriteBufArg(void)
{
    return lastSslWriteBufArg;
}

size_t MbedTlsFake_LastSslWriteLenArg(void)
{
    return lastSslWriteLenArg;
}

void MbedTlsFake_SetSslWriteReturn(int value)
{
    sslWriteReturn = value;
    sslWriteReturnSet = true;
}

int MbedTlsFake_SslReadCallCount(void)
{
    return sslReadCallCount;
}

mbedtls_ssl_context* MbedTlsFake_LastSslReadContextArg(void)
{
    return lastSslReadContextArg;
}

unsigned char* MbedTlsFake_LastSslReadBufArg(void)
{
    return lastSslReadBufArg;
}

size_t MbedTlsFake_LastSslReadLenArg(void)
{
    return lastSslReadLenArg;
}

void MbedTlsFake_SetSslReadReturn(int value)
{
    sslReadReturn = value;
}

int MbedTlsFake_SslCloseNotifyCallCount(void)
{
    return sslCloseNotifyCallCount;
}

mbedtls_ssl_context* MbedTlsFake_LastSslCloseNotifyArg(void)
{
    return lastSslCloseNotifyArg;
}

int MbedTlsFake_SslFreeCallCount(void)
{
    return sslFreeCallCount;
}

mbedtls_ssl_context* MbedTlsFake_LastSslFreeArg(void)
{
    return lastSslFreeArg;
}

int MbedTlsFake_SslConfigFreeCallCount(void)
{
    return sslConfigFreeCallCount;
}

mbedtls_ssl_config* MbedTlsFake_LastSslConfigFreeArg(void)
{
    return lastSslConfigFreeArg;
}

int MbedTlsFake_SslConfAuthmodeCallCount(void)
{
    return sslConfAuthmodeCallCount;
}

mbedtls_ssl_config* MbedTlsFake_LastSslConfAuthmodeConfigArg(void)
{
    return lastSslConfAuthmodeConfigArg;
}

int MbedTlsFake_LastSslConfAuthmodeArg(void)
{
    return lastSslConfAuthmodeArg;
}

int MbedTlsFake_SslConfCaChainCallCount(void)
{
    return sslConfCaChainCallCount;
}

mbedtls_ssl_config* MbedTlsFake_LastSslConfCaChainConfigArg(void)
{
    return lastSslConfCaChainConfigArg;
}

mbedtls_x509_crt* MbedTlsFake_LastSslConfCaChainArg(void)
{
    return lastSslConfCaChainArg;
}

mbedtls_x509_crl* MbedTlsFake_LastSslConfCaChainCrlArg(void)
{
    return lastSslConfCaChainCrlArg;
}

int MbedTlsFake_SslConfRngCallCount(void)
{
    return sslConfRngCallCount;
}

mbedtls_ssl_config* MbedTlsFake_LastSslConfRngConfigArg(void)
{
    return lastSslConfRngConfigArg;
}

int (*MbedTlsFake_LastSslConfRngFuncArg(void))(void*, unsigned char*, size_t)
{
    return lastSslConfRngFuncArg;
}

void* MbedTlsFake_LastSslConfRngContextArg(void)
{
    return lastSslConfRngContextArg;
}

int MbedTlsFake_SslSetHostnameCallCount(void)
{
    return sslSetHostnameCallCount;
}

mbedtls_ssl_context* MbedTlsFake_LastSslSetHostnameContextArg(void)
{
    return lastSslSetHostnameContextArg;
}

const char* MbedTlsFake_LastSslSetHostnameNameArg(void)
{
    return lastSslSetHostnameNameArg;
}

void MbedTlsFake_SetSslSetHostnameReturn(int value)
{
    sslSetHostnameReturn = value;
}

int MbedTlsFake_SslConfOwnCertCallCount(void)
{
    return sslConfOwnCertCallCount;
}

mbedtls_ssl_config* MbedTlsFake_LastSslConfOwnCertConfigArg(void)
{
    return lastSslConfOwnCertConfigArg;
}

mbedtls_x509_crt* MbedTlsFake_LastSslConfOwnCertCertArg(void)
{
    return lastSslConfOwnCertCertArg;
}

mbedtls_pk_context* MbedTlsFake_LastSslConfOwnCertKeyArg(void)
{
    return lastSslConfOwnCertKeyArg;
}

/* -------------------------------------------------------------------------
 * Link-interposed mbedTLS symbols. The test executable does not link
 * libmbedtls; the production code's calls to mbedtls_* resolve here.
 * ------------------------------------------------------------------------- */

void mbedtls_ssl_config_init(mbedtls_ssl_config* conf)
{
    sslConfigInitCallCount++;
    lastSslConfigInitArg = conf;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- signature fixed by mbedTLS API
int mbedtls_ssl_config_defaults(mbedtls_ssl_config* conf, int endpoint, int transport, int preset)
{
    sslConfigDefaultsCallCount++;
    lastSslConfigDefaultsConfigArg = conf;
    lastSslConfigDefaultsEndpoint = endpoint;
    lastSslConfigDefaultsTransport = transport;
    lastSslConfigDefaultsPreset = preset;
    return sslConfigDefaultsReturn;
}

void mbedtls_ssl_init(mbedtls_ssl_context* ssl)
{
    sslInitCallCount++;
    lastSslInitArg = ssl;
}

int mbedtls_ssl_setup(mbedtls_ssl_context* ssl, const mbedtls_ssl_config* conf)
{
    sslSetupCallCount++;
    lastSslSetupContextArg = ssl;
    lastSslSetupConfigArg = conf;
    return sslSetupReturn;
}

void mbedtls_ssl_set_bio(
    mbedtls_ssl_context* ssl,
    void* p_bio,
    mbedtls_ssl_send_t* f_send,
    mbedtls_ssl_recv_t* f_recv,
    mbedtls_ssl_recv_timeout_t* f_recv_timeout
)
{
    sslSetBioCallCount++;
    lastSslSetBioContextArg = ssl;
    lastSslSetBioPBioArg = p_bio;
    lastSslSetBioSendCallback = f_send;
    lastSslSetBioRecvCallback = f_recv;
    lastSslSetBioRecvTimeoutCallback = f_recv_timeout;
}

int mbedtls_ssl_handshake(mbedtls_ssl_context* ssl)
{
    int callIndex = sslHandshakeCallCount;
    sslHandshakeCallCount++;
    lastSslHandshakeArg = ssl;
    int rc = sslHandshakeReturn;
    if (sslHandshakeReturnSequenceLen > 0)
    {
        int idx = (callIndex < sslHandshakeReturnSequenceLen) ? callIndex : (sslHandshakeReturnSequenceLen - 1);
        rc = sslHandshakeReturnSequence[idx];
    }
    return rc;
}

int mbedtls_ssl_write(mbedtls_ssl_context* ssl, const unsigned char* buf, size_t len)
{
    sslWriteCallCount++;
    lastSslWriteContextArg = ssl;
    lastSslWriteBufArg = buf;
    lastSslWriteLenArg = len;
    /* Default: write succeeds fully (returns len). Tests can override via SetSslWriteReturn. */
    return sslWriteReturnSet ? sslWriteReturn : (int) len;
}

int mbedtls_ssl_read(mbedtls_ssl_context* ssl, unsigned char* buf, size_t len)
{
    sslReadCallCount++;
    lastSslReadContextArg = ssl;
    lastSslReadBufArg = buf;
    lastSslReadLenArg = len;
    return sslReadReturn;
}

int mbedtls_ssl_close_notify(mbedtls_ssl_context* ssl)
{
    sslCloseNotifyCallCount++;
    lastSslCloseNotifyArg = ssl;
    return 0;
}

void mbedtls_ssl_free(mbedtls_ssl_context* ssl)
{
    sslFreeCallCount++;
    lastSslFreeArg = ssl;
}

void mbedtls_ssl_config_free(mbedtls_ssl_config* conf)
{
    sslConfigFreeCallCount++;
    lastSslConfigFreeArg = conf;
}

void mbedtls_ssl_conf_authmode(mbedtls_ssl_config* conf, int authmode)
{
    sslConfAuthmodeCallCount++;
    lastSslConfAuthmodeConfigArg = conf;
    lastSslConfAuthmodeArg = authmode;
}

void mbedtls_ssl_conf_ca_chain(mbedtls_ssl_config* conf, mbedtls_x509_crt* ca_chain, mbedtls_x509_crl* ca_crl)
{
    sslConfCaChainCallCount++;
    lastSslConfCaChainConfigArg = conf;
    lastSslConfCaChainArg = ca_chain;
    lastSslConfCaChainCrlArg = ca_crl;
}

int mbedtls_ssl_conf_own_cert(mbedtls_ssl_config* conf, mbedtls_x509_crt* own_cert, mbedtls_pk_context* pk_key)
{
    sslConfOwnCertCallCount++;
    lastSslConfOwnCertConfigArg = conf;
    lastSslConfOwnCertCertArg = own_cert;
    lastSslConfOwnCertKeyArg = pk_key;
    return 0;
}

void mbedtls_ssl_conf_rng(mbedtls_ssl_config* conf, int (*f_rng)(void*, unsigned char*, size_t), void* p_rng)
{
    sslConfRngCallCount++;
    lastSslConfRngConfigArg = conf;
    lastSslConfRngFuncArg = f_rng;
    lastSslConfRngContextArg = p_rng;
}

/* Stub: the production code takes the address of mbedtls_ctr_drbg_random when
 * wiring conf_rng. The fake never actually invokes the function via the
 * captured pointer, so this body never runs at test time. Defined here so the
 * symbol resolves at link time without pulling in real libmbedcrypto. */
// NOLINTNEXTLINE(readability-non-const-parameter) -- signature fixed by mbedTLS API; `output` is an out-buffer the contract writes to
int mbedtls_ctr_drbg_random(void* p_rng, unsigned char* output, size_t output_len)
{
    (void) p_rng;
    (void) output;
    (void) output_len;
    return 0;
}

int mbedtls_ssl_set_hostname(mbedtls_ssl_context* ssl, const char* hostname)
{
    sslSetHostnameCallCount++;
    lastSslSetHostnameContextArg = ssl;
    lastSslSetHostnameNameArg = hostname;
    return sslSetHostnameReturn;
}

int MbedTlsFake_MdHmacCallCount(void)
{
    return mdHmacCallCount;
}

int MbedTlsFake_LastMdInfoType(void)
{
    return lastMdInfoType;
}

const uint8_t* MbedTlsFake_LastMdHmacKey(void)
{
    return lastMdHmacKey;
}

size_t MbedTlsFake_LastMdHmacKeyLen(void)
{
    return lastMdHmacKeyLen;
}

const uint8_t* MbedTlsFake_LastMdHmacInput(void)
{
    return lastMdHmacInput;
}

size_t MbedTlsFake_LastMdHmacInputLen(void)
{
    return lastMdHmacInputLen;
}

void MbedTlsFake_SetMdHmacReturn(int value)
{
    mdHmacReturn = value;
}

int MbedTlsFake_PlatformZeroizeCallCount(void)
{
    return platformZeroizeCallCount;
}

const void* MbedTlsFake_LastPlatformZeroizeBuf(void)
{
    return lastPlatformZeroizeBuf;
}

size_t MbedTlsFake_LastPlatformZeroizeLen(void)
{
    return lastPlatformZeroizeLen;
}

/* Deterministic, NON-cryptographic tag: an FNV-1a fold over the key then the
 * input then each output position. Sensitive to key, input, and position so a
 * changed key, tampered data, or tampered tag all produce a different value —
 * enough to exercise the policy's round-trip / tamper / wrong-key paths without
 * linking real libmbedcrypto. */
void MbedTlsFake_ComputeExpectedTag(
    const uint8_t* key,
    size_t keyLength,
    const uint8_t* input,
    size_t inputLength,
    uint8_t* tagOut
)
{
    uint32_t hash = 2166136261U;
    for (size_t index = 0; index < keyLength; index++)
    {
        hash = (hash ^ key[index]) * 16777619U;
    }
    for (size_t index = 0; index < inputLength; index++)
    {
        hash = (hash ^ input[index]) * 16777619U;
    }
    for (size_t index = 0; index < 32U; index++)
    {
        hash = (hash ^ (uint32_t) index) * 16777619U;
        tagOut[index] = (uint8_t) (hash >> 24U);
    }
}

const mbedtls_md_info_t* mbedtls_md_info_from_type(mbedtls_md_type_t md_type)
{
    static const int mdInfoSentinel = 0;
    lastMdInfoType = (int) md_type;
    return (const mbedtls_md_info_t*) &mdInfoSentinel;
}

int mbedtls_md_hmac(
    const mbedtls_md_info_t* md_info,
    const unsigned char* key,
    size_t keylen,
    const unsigned char* input,
    size_t ilen,
    unsigned char* output
)
{
    (void) md_info;
    mdHmacCallCount++;
    lastMdHmacKeyLen = keylen;
    memcpy(lastMdHmacKey, key, (keylen < sizeof lastMdHmacKey) ? keylen : sizeof lastMdHmacKey);
    lastMdHmacInputLen = ilen;
    memcpy(lastMdHmacInput, input, (ilen < sizeof lastMdHmacInput) ? ilen : sizeof lastMdHmacInput);
    MbedTlsFake_ComputeExpectedTag(key, keylen, input, ilen, output);
    return mdHmacReturn;
}

void mbedtls_platform_zeroize(void* buf, size_t len)
{
    platformZeroizeCallCount++;
    lastPlatformZeroizeBuf = buf;
    lastPlatformZeroizeLen = len;
    memset(buf, 0, len);
}
