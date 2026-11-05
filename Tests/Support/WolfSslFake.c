/* Link-time double for the wolfSSL / wolfCrypt symbols the WolfSsl pack calls.
 * Capture-and-canned-return, never a cipher: it proves the adapter's wiring,
 * and the integration suite proves the cryptography against the real library.
 * Compiled against the real wolfSSL headers so the types match. */
#include "WolfSslFake.h"

#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <string.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/wolfcrypt/hmac.h>
#include <wolfssl/wolfcrypt/memory.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/types.h>

enum
{
    WOLFSSLFAKE_MAX_KEY = 128,
    WOLFSSLFAKE_MAX_INPUT = 512,
    WOLFSSLFAKE_TAG_SIZE = 32,
    WOLFSSLFAKE_GCM_KEY = 32,
    WOLFSSLFAKE_GCM_NONCE = 12,
    WOLFSSLFAKE_MAX_AAD = 64,
    WOLFSSLFAKE_MAX_PLAINTEXT = 256,
    WOLFSSLFAKE_SHA256_SIZE = 32,
    WOLFSSLFAKE_SHA1_SIZE = 20,
    WOLFSSLFAKE_SHA256_DIGEST_BYTE = 0x5A,
    WOLFSSLFAKE_SHA1_DIGEST_BYTE = 0x1A
};

static int hmacInitCallCount;
static int hmacSetKeyCallCount;
static int lastHmacType;
static uint8_t lastHmacKey[WOLFSSLFAKE_MAX_KEY];
static size_t lastHmacKeyLen;
static uint8_t lastHmacInput[WOLFSSLFAKE_MAX_INPUT];
static size_t lastHmacInputLen;
static int hmacFreeCallCount;
static enum WolfSslFakeHmacStep hmacFailStep;

static int aesInitCallCount;
static int aesFreeCallCount;
static int gcmSealCount;
static int gcmOpenCount;
static uint8_t lastGcmKey[WOLFSSLFAKE_GCM_KEY];
static size_t lastGcmKeyLen;
static uint8_t lastGcmNonce[WOLFSSLFAKE_GCM_NONCE];
static uint8_t lastGcmAad[WOLFSSLFAKE_MAX_AAD];
static size_t lastGcmAadLen;
static uint8_t lastGcmPlaintext[WOLFSSLFAKE_MAX_PLAINTEXT];
static size_t lastGcmPlaintextLen;
static enum WolfSslFakeGcmStep gcmFailStep;
static bool gcmAuthFails;

static int rngGenerateBlockCallCount;
static const void* lastRngGenerateBlockContext;
static const void* lastRngGenerateBlockBuf;
static size_t lastRngGenerateBlockLen;
static bool rngGenerateBlockFails;

static int forceZeroCallCount;
static const void* lastForceZeroBuf;
static size_t lastForceZeroLen;

static int hashCallCount;
static enum SolidSyslogTlsHashAlgorithm lastHashAlgorithm;
static const void* lastHashData;
static size_t lastHashDataLen;
static void* lastHashHeap;
static bool sha256Unavailable;
static bool sha1Unavailable;

static int parseCertCallCount;
static const void* lastDecodedCertSource;
static size_t lastDecodedCertSourceLen;
static void* lastDecodedCertHeap;
static int lastParseCertType;
static int lastParseCertVerify;
static void* lastParseCertCm;
static int parseCertReturn;
static int freeDecodedCertCallCount;
static int decodedCertsOutstanding;

void WolfSslFake_Reset(void)
{
    hmacInitCallCount = 0;
    hmacSetKeyCallCount = 0;
    lastHmacType = 0;
    memset(lastHmacKey, 0, sizeof lastHmacKey);
    lastHmacKeyLen = 0U;
    memset(lastHmacInput, 0, sizeof lastHmacInput);
    lastHmacInputLen = 0U;
    hmacFreeCallCount = 0;
    hmacFailStep = WOLFSSLFAKE_HMAC_STEP_NONE;
    aesInitCallCount = 0;
    aesFreeCallCount = 0;
    gcmSealCount = 0;
    gcmOpenCount = 0;
    memset(lastGcmKey, 0, sizeof lastGcmKey);
    lastGcmKeyLen = 0U;
    memset(lastGcmNonce, 0, sizeof lastGcmNonce);
    memset(lastGcmAad, 0, sizeof lastGcmAad);
    lastGcmAadLen = 0U;
    memset(lastGcmPlaintext, 0, sizeof lastGcmPlaintext);
    lastGcmPlaintextLen = 0U;
    gcmFailStep = WOLFSSLFAKE_GCM_STEP_NONE;
    gcmAuthFails = false;
    rngGenerateBlockCallCount = 0;
    lastRngGenerateBlockContext = NULL;
    lastRngGenerateBlockBuf = NULL;
    lastRngGenerateBlockLen = 0U;
    rngGenerateBlockFails = false;
    forceZeroCallCount = 0;
    lastForceZeroBuf = NULL;
    lastForceZeroLen = 0U;
    hashCallCount = 0;
    lastHashAlgorithm = SOLIDSYSLOG_TLS_HASH_SHA256;
    lastHashData = NULL;
    lastHashDataLen = 0U;
    lastHashHeap = NULL;
    sha256Unavailable = false;
    sha1Unavailable = false;
    parseCertCallCount = 0;
    lastDecodedCertSource = NULL;
    lastDecodedCertSourceLen = 0U;
    lastDecodedCertHeap = NULL;
    lastParseCertType = -1;
    lastParseCertVerify = -1;
    lastParseCertCm = NULL;
    parseCertReturn = 0;
    freeDecodedCertCallCount = 0;
    decodedCertsOutstanding = 0;
}

/* -------------------------------------------------------------------------
 * HMAC
 * ------------------------------------------------------------------------- */

void WolfSslFake_ComputeExpectedTag(
    const uint8_t* key,
    size_t keyLength,
    const uint8_t* input,
    size_t inputLength,
    uint8_t* tagOut
)
{
    uint32_t hash = 2166136261U;
    for (size_t index = 0U; index < keyLength; index++)
    {
        hash = (hash ^ key[index]) * 16777619U;
    }
    for (size_t index = 0U; index < inputLength; index++)
    {
        hash = (hash ^ input[index]) * 16777619U;
    }
    for (size_t index = 0U; index < WOLFSSLFAKE_TAG_SIZE; index++)
    {
        hash = (hash ^ (uint32_t) index) * 16777619U;
        tagOut[index] = (uint8_t) (hash >> 24);
    }
}

int wc_HmacInit(Hmac* hmac, void* heap, int devId)
{
    (void) hmac;
    (void) heap;
    (void) devId;
    hmacInitCallCount++;
    return (hmacFailStep == WOLFSSLFAKE_HMAC_STEP_INIT) ? -1 : 0;
}

int wc_HmacSetKey(Hmac* hmac, int type, const byte* key, word32 length)
{
    (void) hmac;
    hmacSetKeyCallCount++;
    lastHmacType = type;
    lastHmacKeyLen = length;
    memcpy(lastHmacKey, key, (length < sizeof lastHmacKey) ? length : sizeof lastHmacKey);
    lastHmacInputLen = 0U;
    return (hmacFailStep == WOLFSSLFAKE_HMAC_STEP_SETKEY) ? -1 : 0;
}

int wc_HmacUpdate(Hmac* hmac, const byte* msg, word32 length)
{
    (void) hmac;
    size_t room = sizeof lastHmacInput - lastHmacInputLen;
    size_t copied = (length < room) ? length : room;
    memcpy(&lastHmacInput[lastHmacInputLen], msg, copied);
    lastHmacInputLen += copied;
    return (hmacFailStep == WOLFSSLFAKE_HMAC_STEP_UPDATE) ? -1 : 0;
}

int wc_HmacFinal(Hmac* hmac, byte* hash)
{
    (void) hmac;
    WolfSslFake_ComputeExpectedTag(lastHmacKey, lastHmacKeyLen, lastHmacInput, lastHmacInputLen, hash);
    return (hmacFailStep == WOLFSSLFAKE_HMAC_STEP_FINAL) ? -1 : 0;
}

void wc_HmacFree(Hmac* hmac)
{
    (void) hmac;
    hmacFreeCallCount++;
}

int WolfSslFake_HmacInitCallCount(void)
{
    return hmacInitCallCount;
}

int WolfSslFake_HmacSetKeyCallCount(void)
{
    return hmacSetKeyCallCount;
}

int WolfSslFake_LastHmacType(void)
{
    return lastHmacType;
}

const uint8_t* WolfSslFake_LastHmacKey(void)
{
    return lastHmacKey;
}

size_t WolfSslFake_LastHmacKeyLen(void)
{
    return lastHmacKeyLen;
}

const uint8_t* WolfSslFake_LastHmacInput(void)
{
    return lastHmacInput;
}

size_t WolfSslFake_LastHmacInputLen(void)
{
    return lastHmacInputLen;
}

int WolfSslFake_HmacFreeCallCount(void)
{
    return hmacFreeCallCount;
}

void WolfSslFake_SetHmacStepFails(enum WolfSslFakeHmacStep step)
{
    hmacFailStep = step;
}

/* -------------------------------------------------------------------------
 * AES-256-GCM - link-interposed wc_Aes* + the RNG nonce source.
 * Capture-and-canned-return, NOT a cipher (see header).
 * ------------------------------------------------------------------------- */

int wc_AesInit(Aes* aes, void* heap, int devId)
{
    (void) aes;
    (void) heap;
    (void) devId;
    aesInitCallCount++;
    return (gcmFailStep == WOLFSSLFAKE_GCM_STEP_INIT) ? -1 : 0;
}

void wc_AesFree(Aes* aes)
{
    (void) aes;
    aesFreeCallCount++;
}

int wc_AesGcmSetKey(Aes* aes, const byte* key, word32 len)
{
    (void) aes;
    lastGcmKeyLen = len;
    memcpy(lastGcmKey, key, (len < sizeof lastGcmKey) ? len : sizeof lastGcmKey);
    return (gcmFailStep == WOLFSSLFAKE_GCM_STEP_SETKEY) ? -1 : 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- signature fixed by the wolfCrypt API
int wc_AesGcmEncrypt(
    Aes* aes,
    byte* out,
    const byte* in,
    word32 sz,
    const byte* iv,
    word32 ivSz,
    byte* authTag,
    word32 authTagSz,
    const byte* authIn,
    word32 authInSz
)
{
    (void) aes;
    gcmSealCount++;
    memcpy(lastGcmNonce, iv, (ivSz < sizeof lastGcmNonce) ? ivSz : sizeof lastGcmNonce);
    /* Report only what was actually captured - a reader walking LastGcmAad() /
     * LastGcmPlaintext() up to the reported length then never runs past the
     * capture buffer if a test exceeds its capacity. */
    lastGcmAadLen = (authInSz < sizeof lastGcmAad) ? authInSz : sizeof lastGcmAad;
    memcpy(lastGcmAad, authIn, lastGcmAadLen);
    lastGcmPlaintextLen = (sz < sizeof lastGcmPlaintext) ? sz : sizeof lastGcmPlaintext;
    memcpy(lastGcmPlaintext, in, lastGcmPlaintextLen);
    /* The double does not encrypt: copy input to output unchanged so the
     * in-place buffer stays defined (memmove - production passes out == in)
     * and write a canned all-zero tag the adapter only forwards into the trailer. */
    memmove(out, in, sz);
    memset(authTag, 0, authTagSz);
    return (gcmFailStep == WOLFSSLFAKE_GCM_STEP_ENCRYPT) ? -1 : 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- signature fixed by the wolfCrypt API
int wc_AesGcmDecrypt(
    Aes* aes,
    byte* out,
    const byte* in,
    word32 sz,
    const byte* iv,
    word32 ivSz,
    const byte* authTag,
    word32 authTagSz,
    const byte* authIn,
    word32 authInSz
)
{
    (void) aes;
    (void) authTag;
    (void) authTagSz;
    gcmOpenCount++;
    memcpy(lastGcmNonce, iv, (ivSz < sizeof lastGcmNonce) ? ivSz : sizeof lastGcmNonce);
    lastGcmAadLen = (authInSz < sizeof lastGcmAad) ? authInSz : sizeof lastGcmAad;
    memcpy(lastGcmAad, authIn, lastGcmAadLen);
    memmove(out, in, sz);
    /* Canned verdict - real tag verification is the integration suite's job.
     * SetGcmAuthFails drives the tamper / wrong-key rejection (silent);
     * the DECRYPT step drives a genuine error (reported DECRYPT_FAILED). */
    int result = 0;
    if (gcmAuthFails)
    {
        result = AES_GCM_AUTH_E;
    }
    else if (gcmFailStep == WOLFSSLFAKE_GCM_STEP_DECRYPT)
    {
        result = -1;
    }
    else
    {
        /* Authentic. */
    }
    return result;
}

int wc_RNG_GenerateBlock(WC_RNG* rng, byte* output, word32 sz)
{
    rngGenerateBlockCallCount++;
    lastRngGenerateBlockContext = rng;
    lastRngGenerateBlockBuf = output;
    lastRngGenerateBlockLen = sz;
    if (rngGenerateBlockFails)
    {
        return -1;
    }
    for (size_t index = 0U; index < sz; index++)
    {
        output[index] = (byte) (0xA0U + index);
    }
    return 0;
}

int WolfSslFake_AesInitCallCount(void)
{
    return aesInitCallCount;
}

int WolfSslFake_AesFreeCallCount(void)
{
    return aesFreeCallCount;
}

int WolfSslFake_GcmSealCount(void)
{
    return gcmSealCount;
}

int WolfSslFake_GcmOpenCount(void)
{
    return gcmOpenCount;
}

const uint8_t* WolfSslFake_LastGcmKey(void)
{
    return lastGcmKey;
}

size_t WolfSslFake_LastGcmKeyLen(void)
{
    return lastGcmKeyLen;
}

const uint8_t* WolfSslFake_LastGcmNonce(void)
{
    return lastGcmNonce;
}

const uint8_t* WolfSslFake_LastGcmAad(void)
{
    return lastGcmAad;
}

size_t WolfSslFake_LastGcmAadLen(void)
{
    return lastGcmAadLen;
}

const uint8_t* WolfSslFake_LastGcmPlaintext(void)
{
    return lastGcmPlaintext;
}

size_t WolfSslFake_LastGcmPlaintextLen(void)
{
    return lastGcmPlaintextLen;
}

void WolfSslFake_SetGcmStepFails(enum WolfSslFakeGcmStep step)
{
    gcmFailStep = step;
}

void WolfSslFake_SetGcmAuthFails(bool fails)
{
    gcmAuthFails = fails;
}

struct WC_RNG* WolfSslFake_Rng(void)
{
    static WC_RNG rng;
    return &rng;
}

int WolfSslFake_RngGenerateBlockCallCount(void)
{
    return rngGenerateBlockCallCount;
}

const void* WolfSslFake_LastRngGenerateBlockContext(void)
{
    return lastRngGenerateBlockContext;
}

const void* WolfSslFake_LastRngGenerateBlockBuf(void)
{
    return lastRngGenerateBlockBuf;
}

size_t WolfSslFake_LastRngGenerateBlockLen(void)
{
    return lastRngGenerateBlockLen;
}

void WolfSslFake_SetRngGenerateBlockFails(bool fails)
{
    rngGenerateBlockFails = fails;
}

/* -------------------------------------------------------------------------
 * Zeroise
 * ------------------------------------------------------------------------- */

void wc_ForceZero(void* mem, size_t len)
{
    forceZeroCallCount++;
    lastForceZeroBuf = mem;
    lastForceZeroLen = len;
    memset(mem, 0, len);
}

int WolfSslFake_ForceZeroCallCount(void)
{
    return forceZeroCallCount;
}

const void* WolfSslFake_LastForceZeroBuf(void)
{
    return lastForceZeroBuf;
}

size_t WolfSslFake_LastForceZeroLen(void)
{
    return lastForceZeroLen;
}

/* -------------------------------------------------------------------------
 * Hash
 * ------------------------------------------------------------------------- */

int WolfSslFake_HashCallCount(void)
{
    return hashCallCount;
}

enum SolidSyslogTlsHashAlgorithm WolfSslFake_LastHashAlgorithm(void)
{
    return lastHashAlgorithm;
}

const void* WolfSslFake_LastHashData(void)
{
    return lastHashData;
}

size_t WolfSslFake_LastHashDataLen(void)
{
    return lastHashDataLen;
}

void WolfSslFake_SetHashUnavailable(enum SolidSyslogTlsHashAlgorithm algorithm)
{
    if (algorithm == SOLIDSYSLOG_TLS_HASH_SHA256)
    {
        sha256Unavailable = true;
    }
    else
    {
        sha1Unavailable = true;
    }
}

void* WolfSslFake_LastHashHeap(void)
{
    return lastHashHeap;
}

int wc_HashGetDigestSize(enum wc_HashType hash_type)
{
    int size = HASH_TYPE_E;
    if ((hash_type == WC_HASH_TYPE_SHA256) && !sha256Unavailable)
    {
        size = WOLFSSLFAKE_SHA256_SIZE;
    }
    else if ((hash_type == WC_HASH_TYPE_SHA) && !sha1Unavailable)
    {
        size = WOLFSSLFAKE_SHA1_SIZE;
    }
    return size;
}

int wc_Hash(enum wc_HashType hash_type, const byte* data, word32 data_len, byte* hash, word32 hash_len)
{
    hashCallCount++;
    lastHashData = data;
    lastHashDataLen = data_len;
    lastHashAlgorithm = (hash_type == WC_HASH_TYPE_SHA) ? SOLIDSYSLOG_TLS_HASH_SHA1 : SOLIDSYSLOG_TLS_HASH_SHA256;
    int size = wc_HashGetDigestSize(hash_type);
    int result = HASH_TYPE_E;
    if ((size > 0) && (hash_len >= (word32) size))
    {
        int fill = (hash_type == WC_HASH_TYPE_SHA) ? WOLFSSLFAKE_SHA1_DIGEST_BYTE : WOLFSSLFAKE_SHA256_DIGEST_BYTE;
        memset(hash, fill, (size_t) size);
        result = 0;
    }
    return result;
}

/* -------------------------------------------------------------------------
 * Decoded certificate
 * ------------------------------------------------------------------------- */

int WolfSslFake_ParseCertCallCount(void)
{
    return parseCertCallCount;
}

const void* WolfSslFake_LastDecodedCertSource(void)
{
    return lastDecodedCertSource;
}

size_t WolfSslFake_LastDecodedCertSourceLen(void)
{
    return lastDecodedCertSourceLen;
}

void* WolfSslFake_LastDecodedCertHeap(void)
{
    return lastDecodedCertHeap;
}

int WolfSslFake_LastParseCertType(void)
{
    return lastParseCertType;
}

int WolfSslFake_LastParseCertVerify(void)
{
    return lastParseCertVerify;
}

void* WolfSslFake_LastParseCertCm(void)
{
    return lastParseCertCm;
}

void WolfSslFake_SetParseCertReturn(int value)
{
    parseCertReturn = value;
}

int WolfSslFake_FreeDecodedCertCallCount(void)
{
    return freeDecodedCertCallCount;
}

int WolfSslFake_DecodedCertsOutstanding(void)
{
    return decodedCertsOutstanding;
}

void wc_InitDecodedCert(struct DecodedCert* cert, const byte* source, word32 inSz, void* heap)
{
    (void) cert;
    lastDecodedCertSource = source;
    lastDecodedCertSourceLen = inSz;
    lastDecodedCertHeap = heap;
    decodedCertsOutstanding++;
}

int wc_ParseCert(struct DecodedCert* cert, int type, int verify, void* cm)
{
    (void) cert;
    parseCertCallCount++;
    lastParseCertType = type;
    lastParseCertVerify = verify;
    lastParseCertCm = cm;
    return parseCertReturn;
}

void wc_FreeDecodedCert(struct DecodedCert* cert)
{
    (void) cert;
    freeDecodedCertCallCount++;
    decodedCertsOutstanding--;
}
