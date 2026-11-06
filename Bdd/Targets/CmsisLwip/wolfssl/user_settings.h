/* wolfSSL build settings for the CMSIS-RTOS2 + lwIP QEMU BDD target.
 *
 * wolfSSL is compiled from source here, the way an embedded integrator builds
 * it: WOLFSSL_USER_SETTINGS on the command line makes every wolfSSL source, and
 * the SolidSyslog wolfSSL pack, read this file in place of the options.h a
 * ./configure build writes. Anything not named keeps wolfSSL's default.
 *
 * OPENSSL_EXTRA is never set: the pack uses none of the compatibility layer.
 */

#ifndef BDD_TARGET_CMSIS_LWIP_WOLFSSL_USER_SETTINGS_H
#define BDD_TARGET_CMSIS_LWIP_WOLFSSL_USER_SETTINGS_H

#include <time.h>

/* --- The port ------------------------------------------------------------
 * wolfSSL's own CMSIS-RTOS2 port supplies its mutexes and its tick. It opens no
 * socket and reads no file: the pack moves the bytes through the I/O callbacks
 * (WOLFSSL_USER_IO) over the injected SolidSyslogStream, and the credentials
 * arrive as buffers baked into the image.
 *
 * The port reads seconds as osKernelGetTickCount() / 1000, which assumes a
 * 1 kHz tick this target does not have. Only session lifetimes are measured
 * with it, and the pack never resumes a session. */
#define WOLFSSL_CMSIS_RTOSv2
/* The port names no thread type of its own, so types.h falls to its default,
 * which decorates a thread callback with __stdcall - a keyword this compiler
 * does not have. wolfSSL starts no thread in this build; the decoration only
 * has to parse. */
#define __stdcall
#define WOLFSSL_USER_IO
#define WOLFSSL_NO_SOCK
#define NO_WRITEV
#define NO_FILESYSTEM
#define NO_DEV_RANDOM
#define NO_MAIN_DRIVER
#define WOLFSSL_IGNORE_FILE_WARN
#define SIZEOF_LONG_LONG 8
#define WOLFSSL_GENERAL_ALIGNMENT 4

/* --- What the pack asks for ------------------------------------------------
 * Each is named, with what its absence costs, on the wolfSSL platform page. */
#define HAVE_TLS_EXTENSIONS
#define HAVE_SNI
#define WOLFSSL_IP_ALT_NAME
#define WOLFSSL_ALWAYS_VERIFY_CB
#define KEEP_PEER_CERT

/* --- Protocol and algorithms -----------------------------------------------
 * TLS 1.2 and 1.3, client only. RSA, finite-field Diffie-Hellman and ECC all
 * stay in, because the stream sets a key-size floor for each. SHA-1 stays in
 * for the sha-1 pin; nothing negotiates it. */
#define NO_OLD_TLS
#define WOLFSSL_TLS13
#define HAVE_HKDF
#define WC_RSA_PSS
#define WC_RSA_BLINDING
#define HAVE_SUPPORTED_CURVES
#define HAVE_FFDHE_2048
/* Sizes the Diffie-Hellman exponent from a table rather than with pow() and
 * log(), which would be this image's only use of floating-point maths. */
#define WOLFSSL_DH_CONST
#define HAVE_ECC
#define ECC_TIMING_RESISTANT
#define HAVE_AESGCM
#define WOLFSSL_SHA384
#define WOLFSSL_SHA512

#define NO_WOLFSSL_SERVER
#define NO_SESSION_CACHE
#define NO_MD4
#define NO_MD5
#define NO_DSA
#define NO_RC4
#define NO_DES3
#define NO_PSK
#define NO_PWDBASED

/* --- Maths ----------------------------------------------------------------
 * Portable C, no assembly: the cost is handshake time, which the lane's
 * handshake budget has to cover on an emulated Cortex-M3. */
#define WOLFSSL_SP_MATH_ALL
#define SP_WORD_SIZE 32

/* --- Memory ---------------------------------------------------------------
 * Every allocation comes from the one static region the target hands wolfSSL
 * (see BddTargetTlsSender_WolfSsl_InnerStream.c). WOLFSSL_NO_MALLOC makes an
 * allocation that reaches neither a heap hint nor the global one fail, rather
 * than fall through to newlib's few kilobytes. WOLFSSL_SMALL_STACK keeps the
 * big-number temporaries off the service task's stack and in that region. */
#define WOLFSSL_STATIC_MEMORY
#define WOLFSSL_NO_MALLOC
#define WOLFSSL_SMALL_STACK

/* --- Seed -----------------------------------------------------------------
 * The Hash-DRBG is wolfSSL's own; only its seed is the target's, and on QEMU
 * that seed is demo-only (see BddTargetWolfSslPort.c). */
#define HAVE_HASHDRBG
int BddTargetWolfSslPort_GenerateSeed(unsigned char* output, unsigned int size);
#define CUSTOM_RAND_GENERATE_SEED BddTargetWolfSslPort_GenerateSeed

/* --- Clock ----------------------------------------------------------------
 * Certificate validity is enforced, on a board with no RTC. USER_TIME makes
 * wolfSSL ask the target for the time, through a function the target defines
 * under the name XTIME (BddTargetWolfSslPort.c), and use its own gmtime. The
 * types are newlib's. */
#define USER_TIME
#define HAVE_TM_TYPE
#define HAVE_TIME_T_TYPE

#endif /* BDD_TARGET_CMSIS_LWIP_WOLFSSL_USER_SETTINGS_H */
