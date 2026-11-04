/* Link-time double for the wolfSSL TLS symbols the WolfSslStream and the PEM
 * buffer credentials call. Capture-and-canned-return, never a handshake: it
 * proves the adapter's wiring, and the integration suite proves the TLS against
 * the real library. Compiled against the real wolfSSL headers so the types
 * match; the opaque WOLFSSL, WOLFSSL_CTX and WOLFSSL_METHOD are stood in for by
 * dummies compared by address. */
#include "WolfSslTlsFake.h"

#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <string.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/settings.h>

enum
{
    WOLFSSLTLSFAKE_MAX_SEQUENCE = 16
};

static int methodDummy;
static int ctxDummy;
static int sessionDummy;
static WOLFSSL_X509_STORE_CTX store;

static int clientMethodExCallCount;
static void* lastClientMethodExHeap;
static bool clientMethodExReturnsNull;
static int ctxNewExCallCount;
static WOLFSSL_METHOD* lastCtxNewExMethod;
static void* lastCtxNewExHeap;
static bool ctxNewExReturnsNull;
static int ctxFreeCallCount;
static WOLFSSL_CTX* lastCtxFreeArg;
static int freesSeenAtCtxFree;

static int newCallCount;
static WOLFSSL_CTX* lastNewCtx;
static bool newReturnsNull;
static int freeCallCount;
static WOLFSSL* lastFreeArg;

static int setMinVersionCallCount;
static WOLFSSL* lastSetMinVersionSsl;
static int lastSetMinVersionArg;
static int setMinVersionReturn;
static int newsSeenAtSetMinVersion;
static int lastMinRsaKeySz;
static int setMinRsaKeySzReturn;
static int lastMinEccKeySz;
static int setMinEccKeySzReturn;
static int lastMinDhKeySz;
static int setMinDhKeySzReturn;

static int checkDomainNameCallCount;
static WOLFSSL* lastCheckDomainNameSsl;
static const char* lastCheckDomainName;
static int checkDomainNameReturn;
static int checkIpAddressCallCount;
static WOLFSSL* lastCheckIpAddressSsl;
static const char* lastCheckIpAddress;
static int checkIpAddressReturn;
static int useSniCallCount;
static WOLFSSL* lastUseSniSsl;
static unsigned char lastUseSniType;
static const void* lastUseSniData;
static unsigned short lastUseSniSize;
static int useSniReturn;

static CallbackIORecv lastIoRecvCallback;
static CallbackIOSend lastIoSendCallback;
static void* lastIoReadCtx;
static void* lastIoWriteCtx;

static int setVerifyCallCount;
static int lastSetVerifyMode;
static VerifyCallback lastSetVerifyCallback;
static void* lastCertCbCtx;

static int connectCallCount;
static WOLFSSL* lastConnectArg;
static int connectErrorSequence[WOLFSSLTLSFAKE_MAX_SEQUENCE];
static int connectErrorSequenceLen;
static int verifyErrors[WOLFSSLTLSFAKE_MAX_SEQUENCE];
static int verifyErrorsLen;
static int verifyDepths[WOLFSSLTLSFAKE_MAX_SEQUENCE];
static int verifyDepthsLen;
static int lastVerifyCallbackResult;
static int verifyCallbackAcceptCount;

/* The chain the peer presents: the leaf, then its issuer. */
static unsigned char leafDer[] = "leaf certificate DER";
static unsigned char issuerDer[] = "issuer certificate DER";
static WOLFSSL_BUFFER_INFO peerChain[2];
static int lastError;

static int writeCallCount;
static WOLFSSL* lastWriteSsl;
static const void* lastWriteData;
static int lastWriteSz;
static bool writeEchoesSize;
static int writeReturn;
static int readCallCount;
static WOLFSSL* lastReadSsl;
static void* lastReadData;
static int lastReadSz;
static int readReturnSequence[WOLFSSLTLSFAKE_MAX_SEQUENCE];
static int readReturnSequenceLen;
static int readError;

static int shutdownCallCount;
static WOLFSSL* lastShutdownArg;

static int loadVerifyBufferCallCount;
static WOLFSSL_CTX* lastLoadVerifyBufferCtx;
static const unsigned char* lastLoadVerifyBufferIn;
static long lastLoadVerifyBufferSz;
static int lastLoadVerifyBufferFormat;
static int loadVerifyBufferReturn;
static int useCertificateChainBufferCallCount;
static const unsigned char* lastUseCertificateChainBufferIn;
static long lastUseCertificateChainBufferSz;
static int useCertificateChainBufferReturn;
static int usePrivateKeyBufferCallCount;
static const unsigned char* lastUsePrivateKeyBufferIn;
static long lastUsePrivateKeyBufferSz;
static int lastUsePrivateKeyBufferFormat;
static int usePrivateKeyBufferReturn;

static void CopySequence(int* destination, int* destinationLen, const int* values, int count)
{
    int safe = (count < WOLFSSLTLSFAKE_MAX_SEQUENCE) ? count : WOLFSSLTLSFAKE_MAX_SEQUENCE;
    for (int i = 0; i < safe; i++)
    {
        destination[i] = values[i];
    }
    *destinationLen = safe;
}

static int SequenceAt(const int* values, int len, int index)
{
    int idx = (index < len) ? index : (len - 1);
    return values[idx];
}

void WolfSslTlsFake_Reset(void)
{
    memset(&store, 0, sizeof store);
    clientMethodExCallCount = 0;
    lastClientMethodExHeap = NULL;
    clientMethodExReturnsNull = false;
    ctxNewExCallCount = 0;
    lastCtxNewExMethod = NULL;
    lastCtxNewExHeap = NULL;
    ctxNewExReturnsNull = false;
    ctxFreeCallCount = 0;
    lastCtxFreeArg = NULL;
    freesSeenAtCtxFree = -1;
    newCallCount = 0;
    lastNewCtx = NULL;
    newReturnsNull = false;
    freeCallCount = 0;
    lastFreeArg = NULL;
    setMinVersionCallCount = 0;
    lastSetMinVersionSsl = NULL;
    lastSetMinVersionArg = 0;
    setMinVersionReturn = WOLFSSL_SUCCESS;
    newsSeenAtSetMinVersion = -1;
    lastMinRsaKeySz = 0;
    setMinRsaKeySzReturn = WOLFSSL_SUCCESS;
    lastMinEccKeySz = 0;
    setMinEccKeySzReturn = WOLFSSL_SUCCESS;
    lastMinDhKeySz = 0;
    setMinDhKeySzReturn = WOLFSSL_SUCCESS;
    checkDomainNameCallCount = 0;
    lastCheckDomainNameSsl = NULL;
    lastCheckDomainName = NULL;
    checkDomainNameReturn = WOLFSSL_SUCCESS;
    checkIpAddressCallCount = 0;
    lastCheckIpAddressSsl = NULL;
    lastCheckIpAddress = NULL;
    checkIpAddressReturn = WOLFSSL_SUCCESS;
    useSniCallCount = 0;
    lastUseSniSsl = NULL;
    lastUseSniType = 0xFFU;
    lastUseSniData = NULL;
    lastUseSniSize = 0U;
    useSniReturn = WOLFSSL_SUCCESS;
    lastIoRecvCallback = NULL;
    lastIoSendCallback = NULL;
    lastIoReadCtx = NULL;
    lastIoWriteCtx = NULL;
    setVerifyCallCount = 0;
    lastSetVerifyMode = -1;
    lastSetVerifyCallback = NULL;
    lastCertCbCtx = NULL;
    connectCallCount = 0;
    lastConnectArg = NULL;
    connectErrorSequence[0] = 0;
    connectErrorSequenceLen = 1;
    verifyErrorsLen = 0;
    verifyDepthsLen = 0;
    lastVerifyCallbackResult = -1;
    verifyCallbackAcceptCount = 0;
    lastError = 0;
    writeCallCount = 0;
    lastWriteSsl = NULL;
    lastWriteData = NULL;
    lastWriteSz = 0;
    writeEchoesSize = true;
    writeReturn = 0;
    readCallCount = 0;
    lastReadSsl = NULL;
    lastReadData = NULL;
    lastReadSz = 0;
    readReturnSequence[0] = 0;
    readReturnSequenceLen = 1;
    readError = WOLFSSL_ERROR_WANT_READ;
    shutdownCallCount = 0;
    lastShutdownArg = NULL;
    loadVerifyBufferCallCount = 0;
    lastLoadVerifyBufferCtx = NULL;
    lastLoadVerifyBufferIn = NULL;
    lastLoadVerifyBufferSz = 0;
    lastLoadVerifyBufferFormat = -1;
    loadVerifyBufferReturn = WOLFSSL_SUCCESS;
    useCertificateChainBufferCallCount = 0;
    lastUseCertificateChainBufferIn = NULL;
    lastUseCertificateChainBufferSz = 0;
    useCertificateChainBufferReturn = WOLFSSL_SUCCESS;
    usePrivateKeyBufferCallCount = 0;
    lastUsePrivateKeyBufferIn = NULL;
    lastUsePrivateKeyBufferSz = 0;
    lastUsePrivateKeyBufferFormat = -1;
    usePrivateKeyBufferReturn = WOLFSSL_SUCCESS;
}

/* -------------------------------------------------------------------------
 * Method and context
 * ------------------------------------------------------------------------- */

struct WOLFSSL_METHOD* WolfSslTlsFake_Method(void)
{
    return (WOLFSSL_METHOD*) &methodDummy;
}

int WolfSslTlsFake_ClientMethodExCallCount(void)
{
    return clientMethodExCallCount;
}

void* WolfSslTlsFake_LastClientMethodExHeap(void)
{
    return lastClientMethodExHeap;
}

void WolfSslTlsFake_SetClientMethodExReturnsNull(bool returnsNull)
{
    clientMethodExReturnsNull = returnsNull;
}

WOLFSSL_METHOD* wolfTLS_client_method_ex(void* heap)
{
    clientMethodExCallCount++;
    lastClientMethodExHeap = heap;
    return clientMethodExReturnsNull ? NULL : WolfSslTlsFake_Method();
}

struct WOLFSSL_CTX* WolfSslTlsFake_Ctx(void)
{
    return (WOLFSSL_CTX*) &ctxDummy;
}

int WolfSslTlsFake_CtxNewExCallCount(void)
{
    return ctxNewExCallCount;
}

struct WOLFSSL_METHOD* WolfSslTlsFake_LastCtxNewExMethod(void)
{
    return lastCtxNewExMethod;
}

void* WolfSslTlsFake_LastCtxNewExHeap(void)
{
    return lastCtxNewExHeap;
}

void WolfSslTlsFake_SetCtxNewExReturnsNull(bool returnsNull)
{
    ctxNewExReturnsNull = returnsNull;
}

WOLFSSL_CTX* wolfSSL_CTX_new_ex(WOLFSSL_METHOD* method, void* heap)
{
    ctxNewExCallCount++;
    lastCtxNewExMethod = method;
    lastCtxNewExHeap = heap;
    return ctxNewExReturnsNull ? NULL : WolfSslTlsFake_Ctx();
}

int WolfSslTlsFake_CtxFreeCallCount(void)
{
    return ctxFreeCallCount;
}

struct WOLFSSL_CTX* WolfSslTlsFake_LastCtxFreeArg(void)
{
    return lastCtxFreeArg;
}

int WolfSslTlsFake_FreesSeenAtCtxFree(void)
{
    return freesSeenAtCtxFree;
}

void wolfSSL_CTX_free(WOLFSSL_CTX* ctx)
{
    ctxFreeCallCount++;
    lastCtxFreeArg = ctx;
    freesSeenAtCtxFree = freeCallCount;
}

/* -------------------------------------------------------------------------
 * Session
 * ------------------------------------------------------------------------- */

struct WOLFSSL* WolfSslTlsFake_Session(void)
{
    return (WOLFSSL*) &sessionDummy;
}

int WolfSslTlsFake_NewCallCount(void)
{
    return newCallCount;
}

struct WOLFSSL_CTX* WolfSslTlsFake_LastNewCtx(void)
{
    return lastNewCtx;
}

void WolfSslTlsFake_SetNewReturnsNull(bool returnsNull)
{
    newReturnsNull = returnsNull;
}

WOLFSSL* wolfSSL_new(WOLFSSL_CTX* ctx)
{
    newCallCount++;
    lastNewCtx = ctx;
    return newReturnsNull ? NULL : WolfSslTlsFake_Session();
}

int WolfSslTlsFake_FreeCallCount(void)
{
    return freeCallCount;
}

struct WOLFSSL* WolfSslTlsFake_LastFreeArg(void)
{
    return lastFreeArg;
}

void wolfSSL_free(WOLFSSL* ssl)
{
    freeCallCount++;
    lastFreeArg = ssl;
}

/* -------------------------------------------------------------------------
 * Floors
 * ------------------------------------------------------------------------- */

int WolfSslTlsFake_SetMinVersionCallCount(void)
{
    return setMinVersionCallCount;
}

struct WOLFSSL* WolfSslTlsFake_LastSetMinVersionSsl(void)
{
    return lastSetMinVersionSsl;
}

int WolfSslTlsFake_LastSetMinVersionArg(void)
{
    return lastSetMinVersionArg;
}

void WolfSslTlsFake_SetSetMinVersionReturn(int value)
{
    setMinVersionReturn = value;
}

int WolfSslTlsFake_NewsSeenAtSetMinVersion(void)
{
    return newsSeenAtSetMinVersion;
}

int wolfSSL_SetMinVersion(WOLFSSL* ssl, int version)
{
    setMinVersionCallCount++;
    lastSetMinVersionSsl = ssl;
    lastSetMinVersionArg = version;
    newsSeenAtSetMinVersion = newCallCount;
    return setMinVersionReturn;
}

int WolfSslTlsFake_LastMinRsaKeySz(void)
{
    return lastMinRsaKeySz;
}

void WolfSslTlsFake_SetSetMinRsaKeySzReturn(int value)
{
    setMinRsaKeySzReturn = value;
}

int wolfSSL_SetMinRsaKey_Sz(WOLFSSL* ssl, short keySz)
{
    (void) ssl;
    lastMinRsaKeySz = keySz;
    return setMinRsaKeySzReturn;
}

int WolfSslTlsFake_LastMinEccKeySz(void)
{
    return lastMinEccKeySz;
}

void WolfSslTlsFake_SetSetMinEccKeySzReturn(int value)
{
    setMinEccKeySzReturn = value;
}

int wolfSSL_SetMinEccKey_Sz(WOLFSSL* ssl, short keySz)
{
    (void) ssl;
    lastMinEccKeySz = keySz;
    return setMinEccKeySzReturn;
}

int WolfSslTlsFake_LastMinDhKeySz(void)
{
    return lastMinDhKeySz;
}

void WolfSslTlsFake_SetSetMinDhKeySzReturn(int value)
{
    setMinDhKeySzReturn = value;
}

int wolfSSL_SetMinDhKey_Sz(WOLFSSL* ssl, word16 keySz_bits)
{
    (void) ssl;
    lastMinDhKeySz = keySz_bits;
    return setMinDhKeySzReturn;
}

/* -------------------------------------------------------------------------
 * Peer name
 * ------------------------------------------------------------------------- */

int WolfSslTlsFake_CheckDomainNameCallCount(void)
{
    return checkDomainNameCallCount;
}

struct WOLFSSL* WolfSslTlsFake_LastCheckDomainNameSsl(void)
{
    return lastCheckDomainNameSsl;
}

const char* WolfSslTlsFake_LastCheckDomainName(void)
{
    return lastCheckDomainName;
}

void WolfSslTlsFake_SetCheckDomainNameReturn(int value)
{
    checkDomainNameReturn = value;
}

int wolfSSL_check_domain_name(WOLFSSL* ssl, const char* dn)
{
    checkDomainNameCallCount++;
    lastCheckDomainNameSsl = ssl;
    lastCheckDomainName = dn;
    return checkDomainNameReturn;
}

int WolfSslTlsFake_CheckIpAddressCallCount(void)
{
    return checkIpAddressCallCount;
}

struct WOLFSSL* WolfSslTlsFake_LastCheckIpAddressSsl(void)
{
    return lastCheckIpAddressSsl;
}

const char* WolfSslTlsFake_LastCheckIpAddress(void)
{
    return lastCheckIpAddress;
}

void WolfSslTlsFake_SetCheckIpAddressReturn(int value)
{
    checkIpAddressReturn = value;
}

int wolfSSL_check_ip_address(WOLFSSL* ssl, const char* ipaddr)
{
    checkIpAddressCallCount++;
    lastCheckIpAddressSsl = ssl;
    lastCheckIpAddress = ipaddr;
    return checkIpAddressReturn;
}

int WolfSslTlsFake_UseSniCallCount(void)
{
    return useSniCallCount;
}

struct WOLFSSL* WolfSslTlsFake_LastUseSniSsl(void)
{
    return lastUseSniSsl;
}

unsigned char WolfSslTlsFake_LastUseSniType(void)
{
    return lastUseSniType;
}

const void* WolfSslTlsFake_LastUseSniData(void)
{
    return lastUseSniData;
}

unsigned short WolfSslTlsFake_LastUseSniSize(void)
{
    return lastUseSniSize;
}

void WolfSslTlsFake_SetUseSniReturn(int value)
{
    useSniReturn = value;
}

int wolfSSL_UseSNI(WOLFSSL* ssl, unsigned char type, const void* data, unsigned short size)
{
    useSniCallCount++;
    lastUseSniSsl = ssl;
    lastUseSniType = type;
    lastUseSniData = data;
    lastUseSniSize = size;
    return useSniReturn;
}

/* -------------------------------------------------------------------------
 * I/O callbacks
 * ------------------------------------------------------------------------- */

int (*WolfSslTlsFake_LastIoRecvCallback(void))(struct WOLFSSL*, char*, int, void*)
{
    return lastIoRecvCallback;
}

int (*WolfSslTlsFake_LastIoSendCallback(void))(struct WOLFSSL*, char*, int, void*)
{
    return lastIoSendCallback;
}

void* WolfSslTlsFake_LastIoReadCtx(void)
{
    return lastIoReadCtx;
}

void* WolfSslTlsFake_LastIoWriteCtx(void)
{
    return lastIoWriteCtx;
}

void wolfSSL_SSLSetIORecv(WOLFSSL* ssl, CallbackIORecv CBIORecv)
{
    (void) ssl;
    lastIoRecvCallback = CBIORecv;
}

void wolfSSL_SSLSetIOSend(WOLFSSL* ssl, CallbackIOSend CBIOSend)
{
    (void) ssl;
    lastIoSendCallback = CBIOSend;
}

void wolfSSL_SetIOReadCtx(WOLFSSL* ssl, void* ctx)
{
    (void) ssl;
    lastIoReadCtx = ctx;
}

void wolfSSL_SetIOWriteCtx(WOLFSSL* ssl, void* ctx)
{
    (void) ssl;
    lastIoWriteCtx = ctx;
}

/* -------------------------------------------------------------------------
 * Verification
 * ------------------------------------------------------------------------- */

int WolfSslTlsFake_SetVerifyCallCount(void)
{
    return setVerifyCallCount;
}

int WolfSslTlsFake_LastSetVerifyMode(void)
{
    return lastSetVerifyMode;
}

int (*WolfSslTlsFake_LastSetVerifyCallback(void))(int, struct WOLFSSL_X509_STORE_CTX*)
{
    return lastSetVerifyCallback;
}

void* WolfSslTlsFake_LastCertCbCtx(void)
{
    return lastCertCbCtx;
}

void wolfSSL_set_verify(WOLFSSL* ssl, int mode, VerifyCallback verify_callback)
{
    (void) ssl;
    setVerifyCallCount++;
    lastSetVerifyMode = mode;
    lastSetVerifyCallback = verify_callback;
}

void wolfSSL_SetCertCbCtx(WOLFSSL* ssl, void* ctx)
{
    (void) ssl;
    lastCertCbCtx = ctx;
}

struct WOLFSSL_X509_STORE_CTX* WolfSslTlsFake_StoreWithError(int error)
{
    return WolfSslTlsFake_StoreAt(error, 0);
}

struct WOLFSSL_X509_STORE_CTX* WolfSslTlsFake_StoreAt(int error, int depth)
{
    memset(&store, 0, sizeof store);
    peerChain[0].buffer = leafDer;
    peerChain[0].length = (word32) sizeof leafDer;
    peerChain[1].buffer = issuerDer;
    peerChain[1].length = (word32) sizeof issuerDer;
    store.error = error;
    store.error_depth = depth;
    store.certs = peerChain;
    store.totalCerts = 2;
    store.userCtx = lastCertCbCtx;
    return &store;
}

const unsigned char* WolfSslTlsFake_LeafDer(void)
{
    return leafDer;
}

size_t WolfSslTlsFake_LeafDerLength(void)
{
    return sizeof leafDer;
}

const unsigned char* WolfSslTlsFake_IssuerDer(void)
{
    return issuerDer;
}

size_t WolfSslTlsFake_IssuerDerLength(void)
{
    return sizeof issuerDer;
}

/* -------------------------------------------------------------------------
 * Handshake
 * ------------------------------------------------------------------------- */

int WolfSslTlsFake_ConnectCallCount(void)
{
    return connectCallCount;
}

struct WOLFSSL* WolfSslTlsFake_LastConnectArg(void)
{
    return lastConnectArg;
}

void WolfSslTlsFake_SetConnectErrorSequence(const int* errors, int count)
{
    CopySequence(connectErrorSequence, &connectErrorSequenceLen, errors, count);
}

void WolfSslTlsFake_SetVerifyErrors(const int* errors, int count)
{
    CopySequence(verifyErrors, &verifyErrorsLen, errors, count);
}

void WolfSslTlsFake_SetVerifyDepths(const int* depths, int count)
{
    CopySequence(verifyDepths, &verifyDepthsLen, depths, count);
}

int WolfSslTlsFake_LastVerifyCallbackResult(void)
{
    return lastVerifyCallbackResult;
}

int WolfSslTlsFake_VerifyCallbackAcceptCount(void)
{
    return verifyCallbackAcceptCount;
}

int wolfSSL_connect(WOLFSSL* ssl)
{
    int callIndex = connectCallCount;
    connectCallCount++;
    lastConnectArg = ssl;
    if ((callIndex == 0) && (lastSetVerifyCallback != NULL))
    {
        for (int i = 0; i < verifyErrorsLen; i++)
        {
            int depth = (i < verifyDepthsLen) ? verifyDepths[i] : 0;
            int preverify = (verifyErrors[i] == 0) ? 1 : 0;
            lastVerifyCallbackResult =
                lastSetVerifyCallback(preverify, WolfSslTlsFake_StoreAt(verifyErrors[i], depth));
            if (lastVerifyCallbackResult != 0)
            {
                verifyCallbackAcceptCount++;
            }
        }
    }
    lastError = SequenceAt(connectErrorSequence, connectErrorSequenceLen, callIndex);
    return (lastError == 0) ? WOLFSSL_SUCCESS : WOLFSSL_FATAL_ERROR;
}

int wolfSSL_get_error(WOLFSSL* ssl, int ret)
{
    (void) ssl;
    (void) ret;
    return lastError;
}

/* -------------------------------------------------------------------------
 * Records
 * ------------------------------------------------------------------------- */

int WolfSslTlsFake_WriteCallCount(void)
{
    return writeCallCount;
}

struct WOLFSSL* WolfSslTlsFake_LastWriteSsl(void)
{
    return lastWriteSsl;
}

const void* WolfSslTlsFake_LastWriteData(void)
{
    return lastWriteData;
}

int WolfSslTlsFake_LastWriteSz(void)
{
    return lastWriteSz;
}

void WolfSslTlsFake_SetWriteReturn(int value)
{
    writeEchoesSize = false;
    writeReturn = value;
}

/* As the library: a NULL session is refused with BAD_FUNC_ARG. */
int wolfSSL_write(WOLFSSL* ssl, const void* data, int sz)
{
    writeCallCount++;
    lastWriteSsl = ssl;
    lastWriteData = data;
    lastWriteSz = sz;
    int rc = writeEchoesSize ? sz : writeReturn;
    if (ssl == NULL)
    {
        rc = BAD_FUNC_ARG;
        lastError = BAD_FUNC_ARG;
    }
    else if (rc <= 0)
    {
        lastError = WOLFSSL_FATAL_ERROR;
    }
    else
    {
        /* Written in full. */
    }
    return rc;
}

int WolfSslTlsFake_ReadCallCount(void)
{
    return readCallCount;
}

struct WOLFSSL* WolfSslTlsFake_LastReadSsl(void)
{
    return lastReadSsl;
}

void* WolfSslTlsFake_LastReadData(void)
{
    return lastReadData;
}

int WolfSslTlsFake_LastReadSz(void)
{
    return lastReadSz;
}

void WolfSslTlsFake_SetReadReturn(int value, int error)
{
    readReturnSequence[0] = value;
    readReturnSequenceLen = 1;
    readError = error;
}

void WolfSslTlsFake_SetReadReturnSequence(const int* values, int count, int error)
{
    CopySequence(readReturnSequence, &readReturnSequenceLen, values, count);
    readError = error;
}

int wolfSSL_read(WOLFSSL* ssl, void* data, int sz)
{
    int callIndex = readCallCount;
    readCallCount++;
    lastReadSsl = ssl;
    lastReadData = data;
    lastReadSz = sz;
    int rc = SequenceAt(readReturnSequence, readReturnSequenceLen, callIndex);
    if (ssl == NULL)
    {
        rc = BAD_FUNC_ARG;
        lastError = BAD_FUNC_ARG;
    }
    else if (rc <= 0)
    {
        lastError = readError;
    }
    else
    {
        /* Bytes delivered. */
    }
    return rc;
}

/* -------------------------------------------------------------------------
 * Shutdown
 * ------------------------------------------------------------------------- */

int WolfSslTlsFake_ShutdownCallCount(void)
{
    return shutdownCallCount;
}

struct WOLFSSL* WolfSslTlsFake_LastShutdownArg(void)
{
    return lastShutdownArg;
}

int wolfSSL_shutdown(WOLFSSL* ssl)
{
    shutdownCallCount++;
    lastShutdownArg = ssl;
    return WOLFSSL_SHUTDOWN_NOT_DONE;
}

/* -------------------------------------------------------------------------
 * Material
 * ------------------------------------------------------------------------- */

int WolfSslTlsFake_LoadVerifyBufferCallCount(void)
{
    return loadVerifyBufferCallCount;
}

struct WOLFSSL_CTX* WolfSslTlsFake_LastLoadVerifyBufferCtx(void)
{
    return lastLoadVerifyBufferCtx;
}

const unsigned char* WolfSslTlsFake_LastLoadVerifyBufferIn(void)
{
    return lastLoadVerifyBufferIn;
}

long WolfSslTlsFake_LastLoadVerifyBufferSz(void)
{
    return lastLoadVerifyBufferSz;
}

int WolfSslTlsFake_LastLoadVerifyBufferFormat(void)
{
    return lastLoadVerifyBufferFormat;
}

void WolfSslTlsFake_SetLoadVerifyBufferReturn(int value)
{
    loadVerifyBufferReturn = value;
}

int wolfSSL_CTX_load_verify_buffer(WOLFSSL_CTX* ctx, const unsigned char* in, long sz, int format)
{
    loadVerifyBufferCallCount++;
    lastLoadVerifyBufferCtx = ctx;
    lastLoadVerifyBufferIn = in;
    lastLoadVerifyBufferSz = sz;
    lastLoadVerifyBufferFormat = format;
    return loadVerifyBufferReturn;
}

int WolfSslTlsFake_UseCertificateChainBufferCallCount(void)
{
    return useCertificateChainBufferCallCount;
}

const unsigned char* WolfSslTlsFake_LastUseCertificateChainBufferIn(void)
{
    return lastUseCertificateChainBufferIn;
}

long WolfSslTlsFake_LastUseCertificateChainBufferSz(void)
{
    return lastUseCertificateChainBufferSz;
}

void WolfSslTlsFake_SetUseCertificateChainBufferReturn(int value)
{
    useCertificateChainBufferReturn = value;
}

int wolfSSL_CTX_use_certificate_chain_buffer(WOLFSSL_CTX* ctx, const unsigned char* in, long sz)
{
    (void) ctx;
    useCertificateChainBufferCallCount++;
    lastUseCertificateChainBufferIn = in;
    lastUseCertificateChainBufferSz = sz;
    return useCertificateChainBufferReturn;
}

int WolfSslTlsFake_UsePrivateKeyBufferCallCount(void)
{
    return usePrivateKeyBufferCallCount;
}

const unsigned char* WolfSslTlsFake_LastUsePrivateKeyBufferIn(void)
{
    return lastUsePrivateKeyBufferIn;
}

long WolfSslTlsFake_LastUsePrivateKeyBufferSz(void)
{
    return lastUsePrivateKeyBufferSz;
}

int WolfSslTlsFake_LastUsePrivateKeyBufferFormat(void)
{
    return lastUsePrivateKeyBufferFormat;
}

void WolfSslTlsFake_SetUsePrivateKeyBufferReturn(int value)
{
    usePrivateKeyBufferReturn = value;
}

int wolfSSL_CTX_use_PrivateKey_buffer(WOLFSSL_CTX* ctx, const unsigned char* in, long sz, int format)
{
    (void) ctx;
    usePrivateKeyBufferCallCount++;
    lastUsePrivateKeyBufferIn = in;
    lastUsePrivateKeyBufferSz = sz;
    lastUsePrivateKeyBufferFormat = format;
    return usePrivateKeyBufferReturn;
}
