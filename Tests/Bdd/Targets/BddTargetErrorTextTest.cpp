#include <stdint.h>

#include "CppUTest/TestHarness.h"

#include "BddTargetErrorText.h"
#include "SolidSyslogDatagramErrors.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPrival.h"

TEST_GROUP(BddTargetErrorText){};

TEST(BddTargetErrorText, ACategoryIsNamedAsTheLibraryNamesIt)
{
    STRCMP_EQUAL("POOL_EXHAUSTED", BddTargetErrorText_Category(SOLIDSYSLOG_CAT_POOL_EXHAUSTED));
}

TEST(BddTargetErrorText, ACategoryTheLibraryDoesNotNameIsUnknown)
{
    STRCMP_EQUAL("unknown", BddTargetErrorText_Category((uint16_t) 0xFFFFU));
}

TEST(BddTargetErrorText, ADetailIsNamedThroughItsSource)
{
    static const struct SolidSyslogErrorSource source = {"test", SolidSyslogDatagramErrors_Name};
    const struct SolidSyslogErrorEvent event = {
        SOLIDSYSLOG_SEVERITY_WARNING,
        &source,
        SOLIDSYSLOG_CAT_DATAGRAM_BASE,
        SOLIDSYSLOG_DATAGRAM_ERROR_NEXT_HOP_UNRESOLVED,
        0
    };

    STRCMP_EQUAL("NEXT_HOP_UNRESOLVED", BddTargetErrorText_Detail(&event));
}

TEST(BddTargetErrorText, ADetailWithoutANameIsADash)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    const struct SolidSyslogErrorEvent event = {SOLIDSYSLOG_SEVERITY_WARNING, &source, 0U, 7, 0};

    STRCMP_EQUAL("-", BddTargetErrorText_Detail(&event));
}
