#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "SolidSyslogDatagram.h"
#include "SolidSyslogVxWorks64Datagram.h"
#include "VxWorks64NetFake.h"

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64Datagram)
{
    struct SolidSyslogDatagram* datagram = nullptr;

    void setup() override
    {
        VxWorks64NetFake_Reset();
        datagram = SolidSyslogVxWorks64Datagram_Create();
    }

    void teardown() override
    {
        SolidSyslogVxWorks64Datagram_Destroy(datagram);
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Datagram, OpenMakesOneSocket)
{
    SolidSyslogDatagram_Open(datagram);

    CALLED_FAKE(VxWorks64NetFake_Socket, ONCE);
}
