#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include <string>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogHeaderFieldPrivate.h"
#include "SolidSyslogVxWorks64Hostname.h"
#include "VxWorks64NetFake.h"
#include "hostLib.h"

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

TEST(SolidSyslogVxWorks64Hostname, WritesNothingWhenGethostnameFails)
{
    VxWorks64NetFake_SetHostname("vxtarget");
    VxWorks64NetFake_FailGethostname();

    SolidSyslogVxWorks64_GetHostname(&field, nullptr);

    STRCMP_EQUAL("", Formatted());
}

TEST(SolidSyslogVxWorks64Hostname, ANameThatFillsTheBufferIsCutToTheLongestName)
{
    const std::string filling(MAXHOSTNAMELEN + 1, 'a');
    VxWorks64NetFake_SetHostname(filling.c_str());

    SolidSyslogVxWorks64_GetHostname(&field, nullptr);

    STRCMP_EQUAL(std::string(MAXHOSTNAMELEN, 'a').c_str(), Formatted());
}
