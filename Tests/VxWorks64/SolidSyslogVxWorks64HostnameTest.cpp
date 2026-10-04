#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "SolidSyslogFormatter.h"
#include "SolidSyslogHeaderFieldPrivate.h"
#include "SolidSyslogVxWorks64Hostname.h"
#include "VxWorks64NetFake.h"

enum
{
    FORMATTER_BUFFER_SIZE = 512
};

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64Hostname)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(FORMATTER_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter = nullptr;
    struct SolidSyslogHeaderField field{};

    void setup() override
    {
        VxWorks64NetFake_Reset();
        formatter = SolidSyslogFormatter_Create(storage, FORMATTER_BUFFER_SIZE);
        SolidSyslogHeaderField_FromFormatter(&field, formatter, FORMATTER_BUFFER_SIZE);
    }

    [[nodiscard]] const char* Formatted() const
    {
        return SolidSyslogFormatter_AsFormattedBuffer(formatter);
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Hostname, WritesTheHostname)
{
    VxWorks64NetFake_SetHostname("vxtarget");

    SolidSyslogVxWorks64_GetHostname(&field, nullptr);

    STRCMP_EQUAL("vxtarget", Formatted());
}
