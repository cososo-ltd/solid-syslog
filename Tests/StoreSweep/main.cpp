#include <cstdio>
#include <string>

#include "StoreSweep.h"
#include "CppUTest/CommandLineTestRunner.h"

int main(int argc, char** argv)
{
    /* The environment is part of the record: a failing tuple reproduces only
     * under the budget and filter it ran with. */
    std::string header = "SolidSyslogStoreSweep: budget=" + std::to_string(StoreSweep_Budget()) +
                         " per fault class (0 = every case), case filter=\"" + StoreSweep_CaseFilter() + "\"\n";
    (void) std::fputs(header.c_str(), stdout);
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
