#include "BddTargetErrorText.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogResolverCategories.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogTlsStreamCategories.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(BddTargetErrorText){};

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
#define CHECK_CATEGORY_TEXT(category, expectedText)                            \
    do                                                                         \
    {                                                                          \
        STRCMP_EQUAL((expectedText), BddTargetErrorText_Category((category))); \
    } while (0)

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

TEST(BddTargetErrorText, BadConfigCategoryMapsToText)
{
    CHECK_CATEGORY_TEXT(SOLIDSYSLOG_CAT_BAD_CONFIG, "bad config");
}

TEST(BddTargetErrorText, BadArgumentCategoryMapsToText)
{
    CHECK_CATEGORY_TEXT(SOLIDSYSLOG_CAT_BAD_ARGUMENT, "bad argument");
}

TEST(BddTargetErrorText, PoolExhaustedCategoryMapsToText)
{
    CHECK_CATEGORY_TEXT(SOLIDSYSLOG_CAT_POOL_EXHAUSTED, "pool exhausted");
}

TEST(BddTargetErrorText, UnknownDestroyCategoryMapsToText)
{
    CHECK_CATEGORY_TEXT(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, "unknown destroy");
}

TEST(BddTargetErrorText, BufferBackendFailedCategoryMapsToText)
{
    CHECK_CATEGORY_TEXT(SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED, "buffer backend failed");
}

TEST(BddTargetErrorText, ResolverResolveFailedCategoryMapsToText)
{
    CHECK_CATEGORY_TEXT(SOLIDSYSLOG_CAT_RESOLVER_RESOLVE_FAILED, "resolve failed");
}

TEST(BddTargetErrorText, SecurityPolicyKeyUnavailableCategoryMapsToText)
{
    CHECK_CATEGORY_TEXT(SOLIDSYSLOG_CAT_SECURITYPOLICY_KEY_UNAVAILABLE, "key unavailable");
}

TEST(BddTargetErrorText, SecurityPolicySealFailedCategoryMapsToText)
{
    CHECK_CATEGORY_TEXT(SOLIDSYSLOG_CAT_SECURITYPOLICY_SEAL_FAILED, "seal failed");
}

TEST(BddTargetErrorText, SecurityPolicyOpenFailedCategoryMapsToText)
{
    CHECK_CATEGORY_TEXT(SOLIDSYSLOG_CAT_SECURITYPOLICY_OPEN_FAILED, "open failed");
}

TEST(BddTargetErrorText, TlsStreamInitFailedCategoryMapsToText)
{
    CHECK_CATEGORY_TEXT(SOLIDSYSLOG_CAT_TLSSTREAM_INIT_FAILED, "TLS init failed");
}

TEST(BddTargetErrorText, TlsStreamHandshakeFailedCategoryMapsToText)
{
    CHECK_CATEGORY_TEXT(SOLIDSYSLOG_CAT_TLSSTREAM_HANDSHAKE_FAILED, "TLS handshake failed");
}

TEST(BddTargetErrorText, UnrecognisedCategoryMapsToUnknown)
{
    CHECK_CATEGORY_TEXT((uint16_t) 0xFFFFU, "unknown");
}
