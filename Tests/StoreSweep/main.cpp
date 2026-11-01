#include <cstdio>

#include "StoreSweep.h"
#include "CppUTest/CommandLineTestRunner.h"

int main(int argc, char** argv)
{
    /* The environment is part of the record: a failing tuple reproduces only
     * under the budget and filter it ran with. */
    std::printf(
        "SolidSyslogStoreSweep: budget=%lu per fault class (0 = every case), case filter=\"%s\"\n",
        static_cast<unsigned long>(StoreSweep_Budget()),
        StoreSweep_CaseFilter().c_str()
    );
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
