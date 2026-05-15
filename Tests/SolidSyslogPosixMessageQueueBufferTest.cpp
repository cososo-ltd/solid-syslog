#include <cstdlib>

#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros
#include "SolidSyslogBuffer.h"
#include "SolidSyslogPosixMessageQueueBuffer.h"
#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogNullStore.h"
#include "SenderFake.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogStore;

static const char* const TEST_MESSAGE = "hello";
static const size_t TEST_MESSAGE_LEN = 5;

// clang-format off
TEST_GROUP(SolidSyslogPosixMessageQueueBuffer)
{
    struct SolidSyslogBuffer* buffer = nullptr;
    char   readData[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    size_t readSize;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        buffer = SolidSyslogPosixMessageQueueBuffer_Create(SOLIDSYSLOG_MAX_MESSAGE_SIZE, 10);
        readSize = 0;
    }

    void teardown() override
    {
        SolidSyslogPosixMessageQueueBuffer_Destroy();
    }

    void Write() const
    {
        SolidSyslogBuffer_Write(buffer, TEST_MESSAGE, TEST_MESSAGE_LEN);
    }

    bool Read()
    {
        return SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    }
};

// clang-format on

TEST(SolidSyslogPosixMessageQueueBuffer, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogPosixMessageQueueBuffer, WriteAndReadReturnsTrue)
{
    Write();
    CHECK_TRUE(Read());
}

TEST(SolidSyslogPosixMessageQueueBuffer, ReadReturnsWrittenData)
{
    Write();
    Read();
    MEMCMP_EQUAL(TEST_MESSAGE, readData, TEST_MESSAGE_LEN);
}

TEST(SolidSyslogPosixMessageQueueBuffer, ReadReturnsWrittenSize)
{
    Write();
    Read();
    LONGS_EQUAL(TEST_MESSAGE_LEN, readSize);
}

TEST(SolidSyslogPosixMessageQueueBuffer, ReadFromEmptyReturnsFalse)
{
    CHECK_FALSE(Read());
}

TEST(SolidSyslogPosixMessageQueueBuffer, MultipleWritesReadInOrder)
{
    SolidSyslogBuffer_Write(buffer, "first", 5);
    SolidSyslogBuffer_Write(buffer, "second", 6);
    Read();
    MEMCMP_EQUAL("first", readData, 5);
    Read();
    MEMCMP_EQUAL("second", readData, 6);
}

TEST(SolidSyslogPosixMessageQueueBuffer, SecondReadAfterSingleWriteReturnsFalse)
{
    Write();
    Read();
    CHECK_FALSE(Read());
}

TEST(SolidSyslogPosixMessageQueueBuffer, ServiceSendsMessageWrittenViaLog)
{
    struct SolidSyslogSender* fakeSender = SenderFake_Create();
    SolidSyslogStore* nullStore = SolidSyslogNullStore_Create();
    SolidSyslogConfig config = {buffer, fakeSender, nullptr, nullptr, nullptr, nullptr, nullStore, nullptr, 0};
    SolidSyslog_Create(&config);

    SolidSyslogMessage message = {SolidSyslogFacility_Local0, SolidSyslogSeverity_Informational, nullptr, nullptr};
    SolidSyslog_Log(&message);
    SolidSyslog_Service();
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);

    SolidSyslog_Destroy();
    SolidSyslogNullStore_Destroy();
    SenderFake_Destroy(fakeSender);
}

IGNORE_TEST(SolidSyslogPosixMessageQueueBuffer, HappyPathOnly)

{
    // Error handling not yet implemented — see Epic #31
    //   Create with zero maxMessageSize or maxMessages
    //   Create when mq_open fails returns NULL
    //   Write with NULL buffer does not crash
    //   Write with NULL data does not crash
    //   Read with NULL buffer does not crash
    //   Read with NULL data does not crash
    //   Read with NULL bytesRead does not crash
    //   Destroy with NULL buffer does not crash
    //   Write when queue is full (back-pressure / overflow)
    //
    // Blocking mode not yet implemented — see S4.5 or later
    //   Read blocks waiting for a message (O_NONBLOCK removed)
}
