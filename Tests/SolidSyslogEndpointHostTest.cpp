#include <stddef.h>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogEndpointHost.h"
#include "SolidSyslogEndpointHostPrivate.h"
#include "SolidSyslogFormatter.h"

enum
{
    TEST_BUFFER_SIZE = 64
};

// clang-format off
TEST_GROUP(SolidSyslogEndpointHost)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(TEST_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter = nullptr;
    struct SolidSyslogEndpointHost host{};

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
        SolidSyslogEndpointHost_FromFormatter(&host, formatter);
    }
};

// clang-format on

TEST(SolidSyslogEndpointHost, StringWritesTheHostVerbatim)
{
    SolidSyslogEndpointHost_String(&host, "logs.example.com", 16);
    STRCMP_EQUAL("logs.example.com", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogEndpointHost, StringOfEmptyHostWritesNothing)
{
    SolidSyslogEndpointHost_String(&host, "", 0);
    STRCMP_EQUAL("", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogEndpointHost, StringIsBoundedByMaxLength)
{
    SolidSyslogEndpointHost_String(&host, "logs.example.com", 4);
    STRCMP_EQUAL("logs", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}
