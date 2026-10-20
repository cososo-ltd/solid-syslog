#include <stdint.h>

#include "CppUTest/TestHarness.h"

#include "SolidSyslogDatagramErrors.h"

TEST_GROUP(SolidSyslogErrorNames){};

TEST(SolidSyslogErrorNames, ADatagramCodeIsNamedAsTheDocumentationNamesIt)
{
    STRCMP_EQUAL("NEXT_HOP_UNRESOLVED", SolidSyslogDatagramErrors_Name(SOLIDSYSLOG_DATAGRAM_ERROR_NEXT_HOP_UNRESOLVED));
}

TEST(SolidSyslogErrorNames, ADatagramCodePastTheEndHasNoName)
{
    POINTERS_EQUAL(nullptr, SolidSyslogDatagramErrors_Name(SOLIDSYSLOG_DATAGRAM_ERROR_MAX));
}

TEST(SolidSyslogErrorNames, ANegativeDatagramCodeHasNoName)
{
    POINTERS_EQUAL(nullptr, SolidSyslogDatagramErrors_Name(-1));
}

TEST(SolidSyslogErrorNames, EveryDatagramCodeHasAName)
{
    for (int32_t code = 0; code < SOLIDSYSLOG_DATAGRAM_ERROR_MAX; code++)
    {
        CHECK_TEXT(SolidSyslogDatagramErrors_Name(code) != nullptr, "a Datagram code has no name");
    }
}
