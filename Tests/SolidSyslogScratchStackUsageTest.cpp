/* Measures what SolidSyslog_Log costs the logging task's stack under each
   scratch, by running it on a thread whose stack is memory this test owns and
   has painted. The sanitiser lane leaves this file out: ASan moves stack arrays
   to its own fake stack, which would hide the very frame being measured. */
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogPassthroughBuffer.h"
#include "SolidSyslogPosixMutex.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSharedScratch.h"
#include "SolidSyslogStackScratch.h"
#include "SolidSyslogTunables.h"

namespace
{
constexpr size_t THREAD_STACK_BYTES = 64U * 1024U;
constexpr uint8_t PAINT = 0xA5U;
/* The shared form must spare the logging task at least this much of the
   message buffer; the rest is tolerance for frame layout and alignment. */
constexpr size_t EXPECTED_SAVING = (SOLIDSYSLOG_MAX_MESSAGE_SIZE * 8U) / 10U;

alignas(4096) uint8_t threadStack[THREAD_STACK_BYTES];
/* Longer than the buffer, so the whole of it is written on every run. */
char longMessage[SOLIDSYSLOG_MAX_MESSAGE_SIZE * 2U];

void* LogOnce(void* handle)
{
    SolidSyslogMessage message =
        {SOLIDSYSLOG_FACILITY_LOCAL0, SOLIDSYSLOG_SEVERITY_INFORMATIONAL, nullptr, longMessage};
    SolidSyslog_Log(static_cast<struct SolidSyslog*>(handle), &message);
    return nullptr;
}

/* The stack grows down from the top of the array, so the deepest point reached
   is the lowest address no longer carrying the paint. */
size_t BytesDisturbed()
{
    size_t untouched = 0U;
    while ((untouched < THREAD_STACK_BYTES) && (threadStack[untouched] == PAINT))
    {
        untouched++;
    }
    return THREAD_STACK_BYTES - untouched;
}
} // namespace

// clang-format off
TEST_GROUP(SolidSyslogScratchStackUsage)
{
    struct SolidSyslogSender* fakeSender = nullptr;
    struct SolidSyslogBuffer* buffer     = nullptr;
    struct SolidSyslogMutex*  mutex      = nullptr;
    struct SolidSyslogScratch* shared    = nullptr;

    void setup() override
    {
        fakeSender = SenderFake_Create();
        buffer     = SolidSyslogPassthroughBuffer_Create(fakeSender);
        mutex      = SolidSyslogPosixMutex_Create();
        shared     = SolidSyslogSharedScratch_Create(mutex);
        memset(longMessage, 'x', sizeof(longMessage) - 1U);
        longMessage[sizeof(longMessage) - 1U] = 0;
    }

    void teardown() override
    {
        SolidSyslogSharedScratch_Destroy(shared);
        SolidSyslogPosixMutex_Destroy(mutex);
        SolidSyslogPassthroughBuffer_Destroy(buffer);
        SenderFake_Destroy(fakeSender);
    }

    void LogOnThePaintedStack(struct SolidSyslog* solidSyslog)
    {
        memset(threadStack, PAINT, sizeof(threadStack));
        pthread_attr_t attributes;
        pthread_t thread;
        LONGS_EQUAL(0, pthread_attr_init(&attributes));
        LONGS_EQUAL(0, pthread_attr_setstack(&attributes, threadStack, sizeof(threadStack)));
        LONGS_EQUAL(0, pthread_create(&thread, &attributes, LogOnce, solidSyslog));
        LONGS_EQUAL(0, pthread_join(thread, nullptr));
        LONGS_EQUAL(0, pthread_attr_destroy(&attributes));
    }

    /* Measured on the second run. The first call through any libc symbol
       binds it lazily, on the calling thread's stack, so a first run is
       deeper by whatever that thread happened to call first. */
    size_t StackUsedLoggingThrough(struct SolidSyslogScratch* scratch)
    {
        SolidSyslogConfig config = {};
        config.Buffer            = buffer;
        config.Sender            = fakeSender;
        config.Scratch           = scratch;
        struct SolidSyslog* solidSyslog = SolidSyslog_Create(&config);

        LogOnThePaintedStack(solidSyslog);
        LogOnThePaintedStack(solidSyslog);

        SolidSyslog_Destroy(solidSyslog);
        return BytesDisturbed();
    }
};

// clang-format on

/* Guards the measurement: if the frame were not visible here, the saving
   below would be asserted against noise. */
TEST(SolidSyslogScratchStackUsage, TheStackScratchPutsTheMessageBufferOnTheLoggingTask)
{
    size_t used = StackUsedLoggingThrough(SolidSyslogStackScratch_Get());
    CHECK_COMPARE(used, >=, SOLIDSYSLOG_MAX_MESSAGE_SIZE);
}

TEST(SolidSyslogScratchStackUsage, TheSharedScratchSparesTheLoggingTaskTheMessageBuffer)
{
    size_t viaStack = StackUsedLoggingThrough(SolidSyslogStackScratch_Get());
    size_t viaShared = StackUsedLoggingThrough(shared);
    CHECK_COMPARE(viaStack - viaShared, >=, EXPECTED_SAVING);
}
