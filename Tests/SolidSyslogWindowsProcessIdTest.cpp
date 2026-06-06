#include "CppUTest/TestHarness.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogHeaderFieldPrivate.h"
#include "SolidSyslogWindowsProcessId.h"
#include "SolidSyslogWindowsProcessIdInternal.h"

#include <windows.h>

enum
{
    FORMATTER_BUFFER_SIZE = 16
};

static DWORD fakePid;

static DWORD WINAPI FakeGetCurrentProcessId(void)
{
    return fakePid;
}

// clang-format off
TEST_GROUP(SolidSyslogWindowsProcessId)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(FORMATTER_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter = nullptr;
    struct SolidSyslogHeaderField field{};

    void setup() override
    {
        fakePid = 4321;
        UT_PTR_SET(WindowsProcessId_GetCurrentProcessId, FakeGetCurrentProcessId);
        formatter = SolidSyslogFormatter_Create(storage, FORMATTER_BUFFER_SIZE);
        SolidSyslogHeaderField_FromFormatter(&field, formatter, FORMATTER_BUFFER_SIZE);
    }

    const char* formatted() const
    {
        return SolidSyslogFormatter_AsFormattedBuffer(formatter);
    }
};

// clang-format on

TEST(SolidSyslogWindowsProcessId, WritesFakePidAsDecimal)
{
    SolidSyslogWindowsProcessId_Get(&field, nullptr);
    STRCMP_EQUAL("4321", formatted());
}

TEST(SolidSyslogWindowsProcessId, WritesZeroWhenPidIsZero)
{
    fakePid = 0;
    SolidSyslogWindowsProcessId_Get(&field, nullptr);
    STRCMP_EQUAL("0", formatted());
}

TEST(SolidSyslogWindowsProcessId, WritesMaxDwordValueAsDecimal)
{
    fakePid = 0xFFFFFFFFU;
    SolidSyslogWindowsProcessId_Get(&field, nullptr);
    STRCMP_EQUAL("4294967295", formatted());
}
