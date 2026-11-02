/* Link-time double for the wolfSSL / wolfCrypt symbols the WolfSsl pack calls.
 * Capture-and-canned-return, never a cipher: it proves the adapter's wiring,
 * and the integration suite proves the cryptography against the real library.
 * Compiled against the real wolfSSL headers so the types match. */
#include "WolfSslFake.h"

#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <string.h>
#include <wolfssl/wolfcrypt/hmac.h>
#include <wolfssl/wolfcrypt/memory.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/types.h>

enum
{
    WOLFSSLFAKE_MAX_KEY = 128,
    WOLFSSLFAKE_MAX_INPUT = 512,
    WOLFSSLFAKE_TAG_SIZE = 32
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

static int forceZeroCallCount;
static const void* lastForceZeroBuf;
static size_t lastForceZeroLen;

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
    forceZeroCallCount = 0;
    lastForceZeroBuf = NULL;
    lastForceZeroLen = 0U;
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
