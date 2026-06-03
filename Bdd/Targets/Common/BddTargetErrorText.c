#include "BddTargetErrorText.h"

#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogResolverCategories.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogTlsStreamCategories.h"

const char* BddTargetErrorText_Category(uint16_t category)
{
    const char* result = "unknown";
    switch (category)
    {
        case SOLIDSYSLOG_CAT_BAD_CONFIG:
            result = "bad config";
            break;
        case SOLIDSYSLOG_CAT_BAD_ARGUMENT:
            result = "bad argument";
            break;
        case SOLIDSYSLOG_CAT_POOL_EXHAUSTED:
            result = "pool exhausted";
            break;
        case SOLIDSYSLOG_CAT_UNKNOWN_DESTROY:
            result = "unknown destroy";
            break;
        case SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED:
            result = "buffer backend failed";
            break;
        case SOLIDSYSLOG_CAT_RESOLVER_RESOLVE_FAILED:
            result = "resolve failed";
            break;
        case SOLIDSYSLOG_CAT_SECURITYPOLICY_KEY_UNAVAILABLE:
            result = "key unavailable";
            break;
        case SOLIDSYSLOG_CAT_SECURITYPOLICY_SEAL_FAILED:
            result = "seal failed";
            break;
        case SOLIDSYSLOG_CAT_SECURITYPOLICY_OPEN_FAILED:
            result = "open failed";
            break;
        case SOLIDSYSLOG_CAT_TLSSTREAM_INIT_FAILED:
            result = "TLS init failed";
            break;
        case SOLIDSYSLOG_CAT_TLSSTREAM_HANDSHAKE_FAILED:
            result = "TLS handshake failed";
            break;
        default:
            break;
    }
    return result;
}
