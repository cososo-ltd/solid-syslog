#include "OpenSslFake.h"

#include <openssl/ssl.h>
#include <stdbool.h>
#include <stddef.h>
#include <openssl/bio.h>
#include <openssl/types.h>

/* -------------------------------------------------------------------------
 * Captured state — one section per OpenSSL API call. Tests read these via
 * accessors below; production reaches libssl through the link-interposed
 * functions at the bottom of the file.
 * ------------------------------------------------------------------------- */

/* Sentinel storage for opaque OpenSSL types — our fake returns stable
 * pointers to these so tests can assert pointer-chain plumbing with
 * POINTERS_EQUAL. */
static char fakeCtxStorage;
static char fakeMethodStorage;
static char fakeSslStorage;
static char fakeBioMethStorage;

/* Pool of fake BIOs. Each slot is independent so callers can exercise
 * multiple BIOs without aliasing. The address of `slot` is the BIO handle
 * returned from BIO_new; `data` is the per-BIO storage that backs
 * BIO_set_data / BIO_get_data. */
typedef struct
{
    char  slot;
    void* data;
} FakeBio;

enum
{
    FAKE_BIO_POOL_SIZE = 4
};

static FakeBio fakeBios[FAKE_BIO_POOL_SIZE];
static int     fakeBioCount;

/* SSL_CTX_new */
static int               ctxNewCallCount;
static const SSL_METHOD* lastCtxNewMethodArg;
static bool              ctxNewFails;

/* SSL_CTX_load_verify_locations */
static SSL_CTX*    lastLoadVerifyLocationsCtxArg;
static const char* lastCaBundlePath;
static bool        loadVerifyLocationsFails;

/* SSL_CTX_set_verify */
static SSL_CTX* lastSetVerifyCtxArg;
static int      lastVerifyMode;

/* SSL_CTX_ctrl (SET_MIN_PROTO_VERSION) */
static SSL_CTX* lastSslCtxCtrlCtxArg;
static long     lastMinProtoVersion;
static bool     minProtoVersionFails;

/* SSL_CTX_set_cipher_list */
static int         setCipherListCallCount;
static SSL_CTX*    lastSetCipherListCtxArg;
static const char* lastCipherList;
static bool        setCipherListFails;

/* SSL_new */
static int      sslNewCallCount;
static SSL_CTX* lastSslNewCtxArg;
static bool     sslNewFails;

/* BIO_meth_new */
static bool bioMethNewFails;

/* BIO_meth_free */
static int         bioMethFreeCallCount;
static BIO_METHOD* lastBioMethFreeArg;

/* BIO_meth_set_read / BIO_meth_set_write */
static BIO_METHOD* lastBioMethSetReadMethodArg;
static int (*lastBioReadCallback)(BIO*, char*, int);
static long (*lastBioCtrlCallback)(BIO*, int, long, void*);
static int (*lastBioCreateCallback)(BIO*);
static int         lastSetInitArg;
static BIO_METHOD* lastBioMethSetWriteMethodArg;
static int (*lastBioWriteCallback)(BIO*, const char*, int);

/* BIO_new */
static int               bioNewCallCount;
static const BIO_METHOD* lastBioNewMethodArg;
static bool              bioNewFails;

/* BIO_set_data / BIO_get_data */
static BIO*  lastSetDataBioArg;
static void* lastSetDataArg;
static BIO*  lastGetDataBioArg;

/* SSL_set_bio */
static int  setBioCallCount;
static SSL* lastSetBioSslArg;
static BIO* lastSetBioReadBioArg;
static BIO* lastSetBioWriteBioArg;

/* SSL_ctrl (SET_TLSEXT_HOSTNAME) */
static SSL*        lastSslCtrlSslArg;
static const char* lastSniHostname;
static bool        sniHostnameFails;

/* SSL_set1_host */
static SSL*        lastSet1HostSslArg;
static const char* lastSet1Host;
static bool        set1HostFails;

/* SSL_connect */
static int  connectCallCount;
static SSL* lastConnectSslArg;
static bool connectFails;

/* SSL_write */
static int         writeCallCount;
static SSL*        lastWriteSslArg;
static const void* lastWriteBuf;
static int         lastWriteSize;
static bool        writeFails;

/* SSL_read */
static int   sslReadCallCount;
static SSL*  lastSslReadSslArg;
static void* lastSslReadBuf;
static int   lastSslReadSize;

/* SSL_shutdown */
static int  shutdownCallCount;
static SSL* lastShutdownSslArg;

/* SSL_free */
static int  freeCallCount;
static SSL* lastFreeSslArg;

/* SSL_CTX_free */
static int      ctxFreeCallCount;
static SSL_CTX* lastCtxFreeCtxArg;

/* SSL_CTX_use_certificate_chain_file */
static int         useCertChainFileCallCount;
static SSL_CTX*    lastUseCertChainFileCtxArg;
static const char* lastClientCertChainPath;
static bool        useCertChainFileFails;

/* SSL_CTX_use_PrivateKey_file */
static int         usePrivateKeyFileCallCount;
static SSL_CTX*    lastUsePrivateKeyFileCtxArg;
static const char* lastClientKeyPath;
static int         lastClientKeyFileType;
static bool        usePrivateKeyFileFails;

/* SSL_CTX_check_private_key */
static int      checkPrivateKeyCallCount;
static SSL_CTX* lastCheckPrivateKeyCtxArg;
static bool     checkPrivateKeyFails;

/* -------------------------------------------------------------------------
 * Reset — zero every captured value.
 * ------------------------------------------------------------------------- */

void OpenSslFake_Reset(void)
{
    ctxNewCallCount               = 0;
    lastCtxNewMethodArg           = NULL;
    ctxNewFails                   = false;
    lastLoadVerifyLocationsCtxArg = NULL;
    lastCaBundlePath              = NULL;
    loadVerifyLocationsFails      = false;
    lastSetVerifyCtxArg           = NULL;
    lastVerifyMode                = 0;
    lastSslCtxCtrlCtxArg          = NULL;
    lastMinProtoVersion           = 0;
    minProtoVersionFails          = false;
    setCipherListCallCount        = 0;
    lastSetCipherListCtxArg       = NULL;
    lastCipherList                = NULL;
    setCipherListFails            = false;
    sslNewCallCount               = 0;
    lastSslNewCtxArg              = NULL;
    sslNewFails                   = false;
    bioMethNewFails               = false;
    bioMethFreeCallCount          = 0;
    lastBioMethFreeArg            = NULL;
    lastBioMethSetReadMethodArg   = NULL;
    lastBioReadCallback           = NULL;
    lastBioCtrlCallback           = NULL;
    lastBioCreateCallback         = NULL;
    lastSetInitArg                = 0;
    lastBioMethSetWriteMethodArg  = NULL;
    lastBioWriteCallback          = NULL;
    bioNewCallCount               = 0;
    lastBioNewMethodArg           = NULL;
    bioNewFails                   = false;
    fakeBioCount                  = 0;
    for (int i = 0; i < FAKE_BIO_POOL_SIZE; i++)
    {
        fakeBios[i].data = NULL;
    }
    lastSetDataBioArg           = NULL;
    lastSetDataArg              = NULL;
    lastGetDataBioArg           = NULL;
    setBioCallCount             = 0;
    lastSetBioSslArg            = NULL;
    lastSetBioReadBioArg        = NULL;
    lastSetBioWriteBioArg       = NULL;
    lastSslCtrlSslArg           = NULL;
    lastSniHostname             = NULL;
    sniHostnameFails            = false;
    lastSet1HostSslArg          = NULL;
    lastSet1Host                = NULL;
    set1HostFails               = false;
    connectCallCount            = 0;
    lastConnectSslArg           = NULL;
    connectFails                = false;
    writeCallCount              = 0;
    lastWriteSslArg             = NULL;
    lastWriteBuf                = NULL;
    lastWriteSize               = 0;
    writeFails                  = false;
    sslReadCallCount            = 0;
    lastSslReadSslArg           = NULL;
    lastSslReadBuf              = NULL;
    lastSslReadSize             = 0;
    shutdownCallCount           = 0;
    lastShutdownSslArg          = NULL;
    freeCallCount               = 0;
    lastFreeSslArg              = NULL;
    ctxFreeCallCount            = 0;
    lastCtxFreeCtxArg           = NULL;
    useCertChainFileCallCount   = 0;
    lastUseCertChainFileCtxArg  = NULL;
    lastClientCertChainPath     = NULL;
    useCertChainFileFails       = false;
    usePrivateKeyFileCallCount  = 0;
    lastUsePrivateKeyFileCtxArg = NULL;
    lastClientKeyPath           = NULL;
    lastClientKeyFileType       = 0;
    usePrivateKeyFileFails      = false;
    checkPrivateKeyCallCount    = 0;
    lastCheckPrivateKeyCtxArg   = NULL;
    checkPrivateKeyFails        = false;
}

/* -------------------------------------------------------------------------
 * Accessors — grouped by the OpenSSL function they describe.
 * ------------------------------------------------------------------------- */

int OpenSslFake_CtxNewCallCount(void)
{
    return ctxNewCallCount;
}

const SSL_METHOD* OpenSslFake_LastCtxNewMethodArg(void)
{
    return lastCtxNewMethodArg;
}

SSL_CTX* OpenSslFake_LastCtxReturned(void)
{
    return (SSL_CTX*) &fakeCtxStorage;
}

SSL_CTX* OpenSslFake_LastLoadVerifyLocationsCtxArg(void)
{
    return lastLoadVerifyLocationsCtxArg;
}

const char* OpenSslFake_LastCaBundlePath(void)
{
    return lastCaBundlePath;
}

SSL_CTX* OpenSslFake_LastSetVerifyCtxArg(void)
{
    return lastSetVerifyCtxArg;
}

int OpenSslFake_LastVerifyMode(void)
{
    return lastVerifyMode;
}

SSL_CTX* OpenSslFake_LastSslCtxCtrlCtxArg(void)
{
    return lastSslCtxCtrlCtxArg;
}

long OpenSslFake_LastMinProtoVersion(void)
{
    return lastMinProtoVersion;
}

int OpenSslFake_SslNewCallCount(void)
{
    return sslNewCallCount;
}

SSL_CTX* OpenSslFake_LastSslNewCtxArg(void)
{
    return lastSslNewCtxArg;
}

SSL* OpenSslFake_LastSslReturned(void)
{
    return (SSL*) &fakeSslStorage;
}

BIO_METHOD* OpenSslFake_LastBioMethReturned(void)
{
    return (BIO_METHOD*) &fakeBioMethStorage;
}

BIO_METHOD* OpenSslFake_LastBioMethSetReadMethodArg(void)
{
    return lastBioMethSetReadMethodArg;
}

int (*OpenSslFake_LastBioReadCallback(void))(BIO*, char*, int)
{
    return lastBioReadCallback;
}

long (*OpenSslFake_LastBioCtrlCallback(void))(BIO*, int, long, void*)
{
    return lastBioCtrlCallback;
}

int (*OpenSslFake_LastBioCreateCallback(void))(BIO*)
{
    return lastBioCreateCallback;
}

int OpenSslFake_LastSetInitArg(void)
{
    return lastSetInitArg;
}

BIO_METHOD* OpenSslFake_LastBioMethSetWriteMethodArg(void)
{
    return lastBioMethSetWriteMethodArg;
}

int (*OpenSslFake_LastBioWriteCallback(void))(BIO*, const char*, int)
{
    return lastBioWriteCallback;
}

int OpenSslFake_BioNewCallCount(void)
{
    return bioNewCallCount;
}

const BIO_METHOD* OpenSslFake_LastBioNewMethodArg(void)
{
    return lastBioNewMethodArg;
}

BIO* OpenSslFake_LastBioReturned(void)
{
    return fakeBioCount > 0 ? (BIO*) &fakeBios[fakeBioCount - 1].slot : NULL;
}

BIO* OpenSslFake_LastSetDataBioArg(void)
{
    return lastSetDataBioArg;
}

void* OpenSslFake_LastSetDataArg(void)
{
    return lastSetDataArg;
}

BIO* OpenSslFake_LastGetDataBioArg(void)
{
    return lastGetDataBioArg;
}

int OpenSslFake_SetBioCallCount(void)
{
    return setBioCallCount;
}

SSL* OpenSslFake_LastSetBioSslArg(void)
{
    return lastSetBioSslArg;
}

BIO* OpenSslFake_LastSetBioReadBioArg(void)
{
    return lastSetBioReadBioArg;
}

BIO* OpenSslFake_LastSetBioWriteBioArg(void)
{
    return lastSetBioWriteBioArg;
}

SSL* OpenSslFake_LastSslCtrlSslArg(void)
{
    return lastSslCtrlSslArg;
}

const char* OpenSslFake_LastSniHostname(void)
{
    return lastSniHostname;
}

SSL* OpenSslFake_LastSet1HostSslArg(void)
{
    return lastSet1HostSslArg;
}

const char* OpenSslFake_LastSet1Host(void)
{
    return lastSet1Host;
}

int OpenSslFake_ConnectCallCount(void)
{
    return connectCallCount;
}

SSL* OpenSslFake_LastConnectSslArg(void)
{
    return lastConnectSslArg;
}

int OpenSslFake_WriteCallCount(void)
{
    return writeCallCount;
}

SSL* OpenSslFake_LastWriteSslArg(void)
{
    return lastWriteSslArg;
}

const void* OpenSslFake_LastWriteBuf(void)
{
    return lastWriteBuf;
}

int OpenSslFake_LastWriteSize(void)
{
    return lastWriteSize;
}

int OpenSslFake_SslReadCallCount(void)
{
    return sslReadCallCount;
}

SSL* OpenSslFake_LastSslReadSslArg(void)
{
    return lastSslReadSslArg;
}

void* OpenSslFake_LastSslReadBuf(void)
{
    return lastSslReadBuf;
}

int OpenSslFake_LastSslReadSize(void)
{
    return lastSslReadSize;
}

int OpenSslFake_ShutdownCallCount(void)
{
    return shutdownCallCount;
}

SSL* OpenSslFake_LastShutdownSslArg(void)
{
    return lastShutdownSslArg;
}

int OpenSslFake_FreeCallCount(void)
{
    return freeCallCount;
}

SSL* OpenSslFake_LastFreeSslArg(void)
{
    return lastFreeSslArg;
}

int OpenSslFake_CtxFreeCallCount(void)
{
    return ctxFreeCallCount;
}

SSL_CTX* OpenSslFake_LastCtxFreeCtxArg(void)
{
    return lastCtxFreeCtxArg;
}

/* -------------------------------------------------------------------------
 * Link-time substitution for OpenSSL — replaces libssl symbols in the test
 * binary. Production links real libssl; tests never link -lssl.
 * Each function records its args for test assertion. Return values are
 * plausible-success stubs; where behaviour needs switching for failure-path
 * tests, add a setter (e.g. OpenSslFake_SetConnectFails) in a later cycle.
 * ------------------------------------------------------------------------- */

const SSL_METHOD* TLS_client_method(void)
{
    return (const SSL_METHOD*) &fakeMethodStorage;
}

SSL_CTX* SSL_CTX_new(const SSL_METHOD* method)
{
    ctxNewCallCount++;
    lastCtxNewMethodArg = method;
    return ctxNewFails ? NULL : (SSL_CTX*) &fakeCtxStorage;
}

void OpenSslFake_SetCtxNewFails(bool fails)
{
    ctxNewFails = fails;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- signature fixed by OpenSSL API
int SSL_CTX_load_verify_locations(SSL_CTX* ctx, const char* CAfile, const char* CApath)
{
    (void) CApath;
    lastLoadVerifyLocationsCtxArg = ctx;
    lastCaBundlePath              = CAfile;
    return loadVerifyLocationsFails ? 0 : 1;
}

void OpenSslFake_SetLoadVerifyLocationsFails(bool fails)
{
    loadVerifyLocationsFails = fails;
}

void SSL_CTX_set_verify(SSL_CTX* ctx, int mode, SSL_verify_cb verify_callback)
{
    (void) verify_callback;
    lastSetVerifyCtxArg = ctx;
    lastVerifyMode      = mode;
}

/* SSL_CTX_set_min_proto_version is a macro forwarding to SSL_CTX_ctrl; fake
 * intercepts the ctrl call for the min-proto command only. */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- signature fixed by OpenSSL API
long SSL_CTX_ctrl(SSL_CTX* ctx, int cmd, long larg, void* parg)
{
    (void) parg;
    lastSslCtxCtrlCtxArg = ctx;
    if (cmd == SSL_CTRL_SET_MIN_PROTO_VERSION)
    {
        lastMinProtoVersion = larg;
        return minProtoVersionFails ? 0 : 1;
    }
    return 1;
}

void OpenSslFake_SetMinProtoVersionFails(bool fails)
{
    minProtoVersionFails = fails;
}

int SSL_CTX_set_cipher_list(SSL_CTX* ctx, const char* str)
{
    setCipherListCallCount++;
    lastSetCipherListCtxArg = ctx;
    lastCipherList          = str;
    return setCipherListFails ? 0 : 1;
}

void OpenSslFake_SetCipherListFails(bool fails)
{
    setCipherListFails = fails;
}

int OpenSslFake_SetCipherListCallCount(void)
{
    return setCipherListCallCount;
}

SSL_CTX* OpenSslFake_LastSetCipherListCtxArg(void)
{
    return lastSetCipherListCtxArg;
}

const char* OpenSslFake_LastCipherList(void)
{
    return lastCipherList;
}

SSL* SSL_new(SSL_CTX* ctx)
{
    sslNewCallCount++;
    lastSslNewCtxArg = ctx;
    return sslNewFails ? NULL : (SSL*) &fakeSslStorage;
}

void OpenSslFake_SetSslNewFails(bool fails)
{
    sslNewFails = fails;
}

BIO_METHOD* BIO_meth_new(int type, const char* name)
{
    (void) type;
    (void) name;
    return bioMethNewFails ? NULL : (BIO_METHOD*) &fakeBioMethStorage;
}

void OpenSslFake_SetBioMethNewFails(bool fails)
{
    bioMethNewFails = fails;
}

void BIO_meth_free(BIO_METHOD* biom)
{
    bioMethFreeCallCount++;
    lastBioMethFreeArg = biom;
}

int OpenSslFake_BioMethFreeCallCount(void)
{
    return bioMethFreeCallCount;
}

BIO_METHOD* OpenSslFake_LastBioMethFreeArg(void)
{
    return lastBioMethFreeArg;
}

int BIO_meth_set_read(BIO_METHOD* biom, int (*read)(BIO*, char*, int))
{
    lastBioMethSetReadMethodArg = biom;
    lastBioReadCallback         = read;
    return 1;
}

int BIO_meth_set_write(BIO_METHOD* biom, int (*write)(BIO*, const char*, int))
{
    lastBioMethSetWriteMethodArg = biom;
    lastBioWriteCallback         = write;
    return 1;
}

int BIO_meth_set_ctrl(BIO_METHOD* biom, long (*ctrl)(BIO*, int, long, void*))
{
    (void) biom;
    lastBioCtrlCallback = ctrl;
    return 1;
}

int BIO_meth_set_create(BIO_METHOD* biom, int (*create)(BIO*))
{
    (void) biom;
    lastBioCreateCallback = create;
    return 1;
}

void BIO_set_init(BIO* a, int init)
{
    (void) a;
    lastSetInitArg = init;
}

BIO* BIO_new(const BIO_METHOD* type)
{
    bioNewCallCount++;
    lastBioNewMethodArg = type;
    BIO* bio            = NULL;
    if (!bioNewFails && fakeBioCount < FAKE_BIO_POOL_SIZE)
    {
        bio = (BIO*) &fakeBios[fakeBioCount].slot;
        fakeBioCount++;
    }
    return bio;
}

void OpenSslFake_SetBioNewFails(bool fails)
{
    bioNewFails = fails;
}

void BIO_set_data(BIO* a, void* ptr)
{
    lastSetDataBioArg = a;
    lastSetDataArg    = ptr;
    for (int i = 0; i < fakeBioCount; i++)
    {
        if ((BIO*) &fakeBios[i].slot == a)
        {
            fakeBios[i].data = ptr;
        }
    }
}

void* BIO_get_data(BIO* a)
{
    lastGetDataBioArg = a;
    void* result      = NULL;
    for (int i = 0; i < fakeBioCount; i++)
    {
        if ((BIO*) &fakeBios[i].slot == a)
        {
            result = fakeBios[i].data;
        }
    }
    return result;
}

void SSL_set_bio(SSL* ssl, BIO* rbio, BIO* wbio)
{
    setBioCallCount++;
    lastSetBioSslArg      = ssl;
    lastSetBioReadBioArg  = rbio;
    lastSetBioWriteBioArg = wbio;
}

/* SSL_set_tlsext_host_name is a macro forwarding to SSL_ctrl; fake intercepts
 * the SET_TLSEXT_HOSTNAME command and captures the hostname pointer. */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- signature fixed by OpenSSL API
long SSL_ctrl(SSL* ssl, int cmd, long larg, void* parg)
{
    (void) larg;
    lastSslCtrlSslArg = ssl;
    if (cmd == SSL_CTRL_SET_TLSEXT_HOSTNAME)
    {
        lastSniHostname = (const char*) parg;
        return sniHostnameFails ? 0 : 1;
    }
    return 1;
}

void OpenSslFake_SetSniHostnameFails(bool fails)
{
    sniHostnameFails = fails;
}

int SSL_set1_host(SSL* ssl, const char* hostname)
{
    lastSet1HostSslArg = ssl;
    lastSet1Host       = hostname;
    return set1HostFails ? 0 : 1;
}

void OpenSslFake_SetSet1HostFails(bool fails)
{
    set1HostFails = fails;
}

int SSL_connect(SSL* ssl)
{
    connectCallCount++;
    lastConnectSslArg = ssl;
    return connectFails ? -1 : 1;
}

void OpenSslFake_SetConnectFails(bool fails)
{
    connectFails = fails;
}

int SSL_write(SSL* ssl, const void* buf, int num)
{
    writeCallCount++;
    lastWriteSslArg = ssl;
    lastWriteBuf    = buf;
    lastWriteSize   = num;
    return writeFails ? -1 : num;
}

void OpenSslFake_SetWriteFails(bool fails)
{
    writeFails = fails;
}

int SSL_read(SSL* ssl, void* buf, int num)
{
    sslReadCallCount++;
    lastSslReadSslArg = ssl;
    lastSslReadBuf    = buf;
    lastSslReadSize   = num;
    return num;
}

int SSL_shutdown(SSL* ssl)
{
    shutdownCallCount++;
    lastShutdownSslArg = ssl;
    return 1;
}

void SSL_free(SSL* ssl)
{
    freeCallCount++;
    lastFreeSslArg = ssl;
}

void SSL_CTX_free(SSL_CTX* ctx)
{
    ctxFreeCallCount++;
    lastCtxFreeCtxArg = ctx;
}

int SSL_CTX_use_certificate_chain_file(SSL_CTX* ctx, const char* file)
{
    useCertChainFileCallCount++;
    lastUseCertChainFileCtxArg = ctx;
    lastClientCertChainPath    = file;
    return useCertChainFileFails ? 0 : 1;
}

int OpenSslFake_UseCertChainFileCallCount(void)
{
    return useCertChainFileCallCount;
}

SSL_CTX* OpenSslFake_LastUseCertChainFileCtxArg(void)
{
    return lastUseCertChainFileCtxArg;
}

const char* OpenSslFake_LastClientCertChainPath(void)
{
    return lastClientCertChainPath;
}

void OpenSslFake_SetUseCertChainFileFails(bool fails)
{
    useCertChainFileFails = fails;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- signature fixed by OpenSSL API
int SSL_CTX_use_PrivateKey_file(SSL_CTX* ctx, const char* file, int type)
{
    usePrivateKeyFileCallCount++;
    lastUsePrivateKeyFileCtxArg = ctx;
    lastClientKeyPath           = file;
    lastClientKeyFileType       = type;
    return usePrivateKeyFileFails ? 0 : 1;
}

int OpenSslFake_UsePrivateKeyFileCallCount(void)
{
    return usePrivateKeyFileCallCount;
}

SSL_CTX* OpenSslFake_LastUsePrivateKeyFileCtxArg(void)
{
    return lastUsePrivateKeyFileCtxArg;
}

const char* OpenSslFake_LastClientKeyPath(void)
{
    return lastClientKeyPath;
}

int OpenSslFake_LastClientKeyFileType(void)
{
    return lastClientKeyFileType;
}

void OpenSslFake_SetUsePrivateKeyFileFails(bool fails)
{
    usePrivateKeyFileFails = fails;
}

int SSL_CTX_check_private_key(const SSL_CTX* ctx)
{
    checkPrivateKeyCallCount++;
    lastCheckPrivateKeyCtxArg = (SSL_CTX*) ctx;
    return checkPrivateKeyFails ? 0 : 1;
}

int OpenSslFake_CheckPrivateKeyCallCount(void)
{
    return checkPrivateKeyCallCount;
}

SSL_CTX* OpenSslFake_LastCheckPrivateKeyCtxArg(void)
{
    return lastCheckPrivateKeyCtxArg;
}

void OpenSslFake_SetCheckPrivateKeyFails(bool fails)
{
    checkPrivateKeyFails = fails;
}
