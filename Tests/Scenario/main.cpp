#include <cstdint>
#include <cstdio>
#include <string>

#include "ScenarioSuite.h"
#include "CppUTest/CommandLineTestRunner.h"

int main(int argc, char** argv)
{
    /* The environment is part of the record: a failing walk reproduces from
     * its seed alone. */
    uint32_t seed = 0U;
    std::string walks = ScenarioSuite_SeedGiven(&seed) ? ("seed=" + std::to_string(seed) + " only")
                                                       : ("runs=" + std::to_string(ScenarioSuite_Runs()));
    std::string header = "SolidSyslogScenarioSuite: random walks " + walks + "\n";
    (void) std::fputs(header.c_str(), stdout);
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
