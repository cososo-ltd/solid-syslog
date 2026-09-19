#include "SolidSyslogCmsisRtosSysUpTimeTestHelper.h"

/* Whitebox include: SolidSyslogCmsisRtosSysUpTime.c is compiled into this test
   translation unit so its file-scope rollover state is directly reachable. The
   test executable compiles this file in place of the adapter, so there is one
   definition of each. */
// NOLINTNEXTLINE(bugprone-suspicious-include)
#include "SolidSyslogCmsisRtosSysUpTime.c"

void TestCmsisRtosSysUpTime_Reset(void)
{
    CmsisRtosSysUpTime_LastTicks = 0;
    CmsisRtosSysUpTime_Rollovers = 0;
}
