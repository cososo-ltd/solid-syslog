#include <stdint.h>

#include "BddTargetStderrErrorHandler.h"
#include "CppUTest/TestHarness.h"
#include "SolidSyslogError.h"
#include "SolidSyslogLoggerCategories.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStoreCategories.h"

// clang-format off
TEST_GROUP(BddTargetStderrErrorHandler)
{
    void teardown() override
    {
        BddTargetStderrErrorHandler_SetByName("errors-fatal", "1");
    }
};

// clang-format on

TEST(BddTargetStderrErrorHandler, SetByNameTakesErrorsFatalOff)
{
    CHECK_TRUE(BddTargetStderrErrorHandler_SetByName("errors-fatal", "0"));
}

TEST(BddTargetStderrErrorHandler, SetByNameTakesErrorsFatalOn)
{
    CHECK_TRUE(BddTargetStderrErrorHandler_SetByName("errors-fatal", "1"));
}

TEST(BddTargetStderrErrorHandler, SetByNameRejectsAValueThatIsNeither)
{
    CHECK_FALSE(BddTargetStderrErrorHandler_SetByName("errors-fatal", "yes"));
}

TEST(BddTargetStderrErrorHandler, SetByNameRejectsANameItDoesNotOwn)
{
    CHECK_FALSE(BddTargetStderrErrorHandler_SetByName("tls-ca", "ca"));
}

static struct SolidSyslogErrorEvent EventOf(enum SolidSyslogSeverity severity, uint16_t category)
{
    struct SolidSyslogErrorEvent event = {};
    event.Severity = severity;
    event.Category = category;
    return event;
}

TEST(BddTargetStderrErrorHandler, AnErrorIsFatal)
{
    struct SolidSyslogErrorEvent event = EventOf(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_CAT_STORE_FROZEN);

    CHECK_TRUE(BddTargetStderrErrorHandler_IsFatal(&event));
}

TEST(BddTargetStderrErrorHandler, AWarningIsNotFatal)
{
    struct SolidSyslogErrorEvent event = EventOf(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_CAT_STORE_RECORDS_LOST);

    CHECK_FALSE(BddTargetStderrErrorHandler_IsFatal(&event));
}

TEST(BddTargetStderrErrorHandler, NoErrorIsFatalWithErrorsFatalOff)
{
    struct SolidSyslogErrorEvent event = EventOf(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_CAT_STORE_FROZEN);
    BddTargetStderrErrorHandler_SetByName("errors-fatal", "0");

    CHECK_FALSE(BddTargetStderrErrorHandler_IsFatal(&event));
}

/* The halt callback is the target's reaction to a halt, and it runs after the
 * halt is raised, so a halt that ended the target would never reach it. */
TEST(BddTargetStderrErrorHandler, AHaltIsNotFatal)
{
    struct SolidSyslogErrorEvent event = EventOf(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_CAT_LOGGER_HALTED);

    CHECK_FALSE(BddTargetStderrErrorHandler_IsFatal(&event));
}
