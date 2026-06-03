#include "BddTargetErrorText.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogResolverCategories.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogTlsStreamCategories.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(BddTargetErrorText){};

TEST(BddTargetErrorText, BadConfigCategoryMapsToText)
{
    STRCMP_EQUAL("bad config", BddTargetErrorText_Category(SOLIDSYSLOG_CAT_BAD_CONFIG));
}

TEST(BddTargetErrorText, BadArgumentCategoryMapsToText)
{
    STRCMP_EQUAL("bad argument", BddTargetErrorText_Category(SOLIDSYSLOG_CAT_BAD_ARGUMENT));
}

TEST(BddTargetErrorText, PoolExhaustedCategoryMapsToText)
{
    STRCMP_EQUAL("pool exhausted", BddTargetErrorText_Category(SOLIDSYSLOG_CAT_POOL_EXHAUSTED));
}

TEST(BddTargetErrorText, UnknownDestroyCategoryMapsToText)
{
    STRCMP_EQUAL("unknown destroy", BddTargetErrorText_Category(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY));
}

TEST(BddTargetErrorText, BufferBackendFailedCategoryMapsToText)
{
    STRCMP_EQUAL("buffer backend failed", BddTargetErrorText_Category(SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED));
}

TEST(BddTargetErrorText, ResolverResolveFailedCategoryMapsToText)
{
    STRCMP_EQUAL("resolve failed", BddTargetErrorText_Category(SOLIDSYSLOG_CAT_RESOLVER_RESOLVE_FAILED));
}

TEST(BddTargetErrorText, SecurityPolicyKeyUnavailableCategoryMapsToText)
{
    STRCMP_EQUAL("key unavailable", BddTargetErrorText_Category(SOLIDSYSLOG_CAT_SECURITYPOLICY_KEY_UNAVAILABLE));
}

TEST(BddTargetErrorText, SecurityPolicySealFailedCategoryMapsToText)
{
    STRCMP_EQUAL("seal failed", BddTargetErrorText_Category(SOLIDSYSLOG_CAT_SECURITYPOLICY_SEAL_FAILED));
}

TEST(BddTargetErrorText, SecurityPolicyOpenFailedCategoryMapsToText)
{
    STRCMP_EQUAL("open failed", BddTargetErrorText_Category(SOLIDSYSLOG_CAT_SECURITYPOLICY_OPEN_FAILED));
}

TEST(BddTargetErrorText, TlsStreamInitFailedCategoryMapsToText)
{
    STRCMP_EQUAL("TLS init failed", BddTargetErrorText_Category(SOLIDSYSLOG_CAT_TLSSTREAM_INIT_FAILED));
}

TEST(BddTargetErrorText, TlsStreamHandshakeFailedCategoryMapsToText)
{
    STRCMP_EQUAL("TLS handshake failed", BddTargetErrorText_Category(SOLIDSYSLOG_CAT_TLSSTREAM_HANDSHAKE_FAILED));
}

TEST(BddTargetErrorText, UnrecognisedCategoryMapsToUnknown)
{
    STRCMP_EQUAL("unknown", BddTargetErrorText_Category((uint16_t) 0xFFFFU));
}
