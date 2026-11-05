#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/TestPlugin.h"
#include "CppUTest/TestRegistry.h"

#include "SolidSyslogPendingEvents.h"

/* Every test starts with nothing pending in Core's static event table. Only
 * destroying a logger clears it, and many tests raise a fault with no logger
 * at all, so without this the next test's Service would report it. */
class PendingEventsClearPlugin : public TestPlugin
{
  public:
    PendingEventsClearPlugin() : TestPlugin("PendingEventsClear")
    {
    }

    void preTestAction(UtestShell& /*test*/, TestResult& /*result*/) override
    {
        SolidSyslogPendingEvents_Clear();
    }
};

int main(int argc, char** argv)
{
    PendingEventsClearPlugin clear;
    TestRegistry::getCurrentRegistry()->installPlugin(&clear);
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
