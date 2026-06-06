#include "CppUTest/TestHarness.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogHeaderFieldPrivate.h"
#include "SolidSyslogWindowsHostname.h"
#include "SolidSyslogWindowsHostnameInternal.h"

#include <string.h>
#include <windows.h>

enum
{
    FORMATTER_BUFFER_SIZE = 256
};

static const char* fakeHostname;
static BOOL fakeReturnValue;

static BOOL WINAPI FakeGetComputerNameExA(COMPUTER_NAME_FORMAT nameType, LPSTR buffer, LPDWORD size)
{
    (void) nameType;
    if (!fakeReturnValue)
    {
        return FALSE;
    }
    const size_t length = strlen(fakeHostname);
    if ((length + 1U) > *size)
    {
        *size = (DWORD) (length + 1U);
        return FALSE;
    }
    memcpy(buffer, fakeHostname, length);
    buffer[length] = '\0';
    *size = (DWORD) length;
    return TRUE;
}

// clang-format off
TEST_GROUP(SolidSyslogWindowsHostname)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(FORMATTER_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter = nullptr;
    struct SolidSyslogHeaderField field{};

    void setup() override
    {
        fakeHostname    = "winhost";
        fakeReturnValue = TRUE;
        UT_PTR_SET(WindowsHostname_GetComputerNameExA, FakeGetComputerNameExA);
        formatter = SolidSyslogFormatter_Create(storage, FORMATTER_BUFFER_SIZE);
        SolidSyslogHeaderField_FromFormatter(&field, formatter, FORMATTER_BUFFER_SIZE);
    }

    const char* formatted() const
    {
        return SolidSyslogFormatter_AsFormattedBuffer(formatter);
    }
};

// clang-format on

TEST(SolidSyslogWindowsHostname, WritesFakeHostnameIntoFormatter)
{
    SolidSyslogWindowsHostname_Get(&field, nullptr);
    STRCMP_EQUAL("winhost", formatted());
}

TEST(SolidSyslogWindowsHostname, WritesNothingWhenApiFails)
{
    fakeReturnValue = FALSE;
    SolidSyslogWindowsHostname_Get(&field, nullptr);
    STRCMP_EQUAL("", formatted());
}

TEST(SolidSyslogWindowsHostname, EmptyHostnameProducesEmptyString)
{
    fakeHostname = "";
    SolidSyslogWindowsHostname_Get(&field, nullptr);
    STRCMP_EQUAL("", formatted());
}

TEST(SolidSyslogWindowsHostname, HostnameTooLongForBufferProducesEmptyString)
{
    // 260-char hostname exceeds the internal 256-char buffer. GetComputerNameExA
    // reports ERROR_MORE_DATA → the helper treats this as failure → nothing written.
    static char longName[261];
    memset(longName, 'x', 260);
    longName[260] = '\0';
    fakeHostname = longName;
    SolidSyslogWindowsHostname_Get(&field, nullptr);
    STRCMP_EQUAL("", formatted());
}
