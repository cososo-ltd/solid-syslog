# wolfSSL setup

Wiring the wolfSSL at-rest policies so a `SolidSyslogBlockStore` seals every
record it keeps and verifies it on replay. What this pack needs and reports is
on the [wolfSSL](index.md) page. Every config field is documented on its
struct. This page is the order to wire them in.

## What to link

```cmake
set(SOLIDSYSLOG_PLATFORMS "WolfSsl;<Network>;<File>;<OsPrimitives>")
target_link_libraries(my_app PRIVATE SolidSyslog SolidSyslog::WolfSsl)
```

The placeholders are whichever platforms the [capability matrix](../index.md)
says fill the rest of what your build needs. See
[naming your platforms](../../build-integration.md#cmake) for how the list is
read.

## What you need

- wolfSSL built your way, with its include directory on your target. If it was
  built against a `user_settings.h`, define `WOLFSSL_USER_SETTINGS` on your
  target as you do for the library, so the adapter reads the same file.
- For AES-GCM, a `WC_RNG` you have initialised with `wc_InitRng`, which must
  outlive the policy.
- A key accessor: a `SolidSyslogKeyFunction` that copies the key into the
  buffer it is handed and reports its length. The policy fetches the key for
  every seal and open and wipes the copy afterwards, so the accessor is where
  the key lives, and a secure element or a key store sits behind it.

The adapter sources compile in your target, so add them to your build as
[adding it to your build](../../build-integration.md) describes.

```c
#include <wolfssl/wolfcrypt/random.h>

#include "SolidSyslogBlockStore.h"
#include "SolidSyslogWolfSslAesGcmPolicy.h"
#include "SolidSyslogWolfSslHmacSha256Policy.h"
```

Include `wolfssl/options.h` before any wolfSSL header in your own sources when
your library came from `./configure`; the adapter does the same inside.

## 1. Install an error handler

Every fault below reaches `SolidSyslog_SetErrorHandler` and nowhere else. Install
one before creating the policy, or a bad configuration degrades to the Null
policy and the store seals nothing.

## 2. A key accessor

```c
static bool GetKey(void* context, uint8_t* keyOut, size_t capacity, size_t* keyLengthOut)
{
    (void) context;
    if (capacity < 32U)
    {
        return false;
    }
    /* Copy the 32-byte key from wherever it is kept. */
    *keyLengthOut = 32U;
    return true;
}
```

The HMAC policy takes a key of 32 to `SOLIDSYSLOG_MAX_HMAC_KEY_SIZE` bytes and
treats a shorter one as unavailable. The AES-GCM policy takes exactly 32.

## 3. The policy

```c
static WC_RNG rng;
wc_InitRng(&rng);

static const struct SolidSyslogWolfSslAesGcmPolicyConfig aesConfig = {GetKey, NULL, &rng};
struct SolidSyslogSecurityPolicy* policy = SolidSyslogWolfSslAesGcmPolicy_Create(&aesConfig);
```

or, for integrity without confidentiality,

```c
static const struct SolidSyslogWolfSslHmacSha256PolicyConfig hmacConfig = {GetKey, NULL};
struct SolidSyslogSecurityPolicy* policy = SolidSyslogWolfSslHmacSha256Policy_Create(&hmacConfig);
```

Create returns the shared Null policy on a NULL field or an exhausted pool, and
reports which. Wire the handle into `SolidSyslogBlockStoreConfig.SecurityPolicy`.

## Memory

The adapter allocates nothing itself. Each seal and open builds one wolfCrypt
context on the stack and frees it before returning, so the cost is what
wolfCrypt's `Hmac` or `Aes` needs under your build. The key copy lives on the
stack for the one call and is wiped with `wc_ForceZero`.

## Teardown

Destroy the store before the policy, and the policy before freeing the RNG
with `wc_FreeRng`. The policy borrows the RNG and the store borrows the policy.

## What the handler sees

`event->Source` is `&SolidSyslogWolfSslHmacSha256PolicyErrorSource` or
`&SolidSyslogWolfSslAesGcmPolicyErrorSource`. `event->Detail` is a code from
`SolidSyslogHmacSha256PolicyErrors.h` or `SolidSyslogAesGcmPolicyErrors.h`; the
`SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_` and `SOLIDSYSLOG_AES_GCM_POLICY_ERROR_`
prefixes are dropped below.

| What happened | Severity | Category | Detail | Then |
|---|---|---|---|---|
| config NULL, `GetKey` NULL, or `Rng` NULL for AES-GCM | `CRITICAL` | `BAD_CONFIG` | `BAD_CONFIG` | Null policy returned; the store seals nothing |
| the pool is full | `CRITICAL` | `POOL_EXHAUSTED` | `POOL_EXHAUSTED` | Null policy returned |
| Destroy of a handle not from the pool | `WARNING` | `UNKNOWN_DESTROY` | `UNKNOWN_DESTROY` | ignored |
| `GetKey` returned false, or an AES key not 32 bytes | `ERROR` | `SECURITY_POLICY_KEY_UNAVAILABLE` | `KEY_UNAVAILABLE` | the seal fails and nothing is stored; an open could not run and the record waits |
| an HMAC key shorter than 32 bytes or longer than the buffer | `ERROR` | `SECURITY_POLICY_KEY_UNAVAILABLE` | `KEY_TOO_SHORT` | as above |
| wolfCrypt failed to compute the HMAC | `ERROR` | `SECURITY_POLICY_SEAL_FAILED` or `SECURITY_POLICY_OPEN_FAILED` | `HMAC_FAILED` | as above |
| the RNG would not supply a nonce | `ERROR` | `SECURITY_POLICY_SEAL_FAILED` | `NONCE_FAILED` | the seal fails and nothing is stored |
| wolfCrypt failed to encrypt or decrypt | `ERROR` | `SECURITY_POLICY_SEAL_FAILED` or `SECURITY_POLICY_OPEN_FAILED` | `ENCRYPT_FAILED`, `DECRYPT_FAILED` | as above |

A record that fails to verify, because it was altered or sealed under another
key, is rejected without a report: that verdict is the policy doing its job,
and the store reports the loss. A run of faults is reported once, and the next
seal or open that reaches a verdict re-arms it.
