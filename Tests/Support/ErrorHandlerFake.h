#ifndef ERRORHANDLERFAKE_H
#define ERRORHANDLERFAKE_H

#include "ExternC.h"
#include "SolidSyslogPrival.h"

EXTERN_C_BEGIN

    void                      ErrorHandlerFake_Install(void* context);
    void                      ErrorHandlerFake_Uninstall(void);
    int                       ErrorHandlerFake_HandleCallCount(void);
    enum SolidSyslog_Severity ErrorHandlerFake_LastSeverity(void);
    const char*               ErrorHandlerFake_LastMessage(void);
    const void*               ErrorHandlerFake_LastContext(void);

EXTERN_C_END

#ifdef __cplusplus
#include "CppUTest/TestHarness.h"
#include "TestUtils.h"

/* Assertion macros for tests that observe SolidSyslog_Error through the fake.
 * Callers must bring CALLED_FAKE / ONCE / NEVER into scope with
 * `using namespace CososoTesting;` (existing convention in this codebase). */
// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while) -- macros preserve __FILE__/__LINE__ in test failure output
#define CHECK_REPORTED_ERROR(expectedMessage)                                   \
    do                                                                          \
    {                                                                           \
        CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);                             \
        LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERR, ErrorHandlerFake_LastSeverity()); \
        STRCMP_EQUAL((expectedMessage), ErrorHandlerFake_LastMessage());        \
    } while (0)

#define CHECK_NOTHING_REPORTED() CALLED_FAKE(ErrorHandlerFake_Handle, NEVER)
// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
#endif

#endif /* ERRORHANDLERFAKE_H */
