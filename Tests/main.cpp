#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/TestPlugin.h"
#include "CppUTest/TestRegistry.h"

#include "PendingEventsDrain.h"

/* Every test starts with nothing pending in Core's static event table, so a
 * fault one test raised is never reported by the next test's Service. */
class PendingEventsDrainPlugin : public TestPlugin
{
  public:
    PendingEventsDrainPlugin() : TestPlugin("PendingEventsDrain")
    {
    }

    void preTestAction(UtestShell& /*test*/, TestResult& /*result*/) override
    {
        PendingEventsDrain_All();
    }
};

int main(int argc, char** argv)
{
    PendingEventsDrainPlugin drain;
    TestRegistry::getCurrentRegistry()->installPlugin(&drain);
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
