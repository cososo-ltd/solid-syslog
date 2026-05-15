#include "SolidSyslogPrival.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(SolidSyslogPrival){};

TEST(SolidSyslogPrival, FacilityEnumValuesMatchRfc5424)
{
    LONGS_EQUAL(0, SolidSyslogFacility_Kern);
    LONGS_EQUAL(1, SolidSyslogFacility_User);
    LONGS_EQUAL(2, SolidSyslogFacility_Mail);
    LONGS_EQUAL(3, SolidSyslogFacility_Daemon);
    LONGS_EQUAL(4, SolidSyslogFacility_Auth);
    LONGS_EQUAL(5, SolidSyslogFacility_Syslog);
    LONGS_EQUAL(6, SolidSyslogFacility_Lpr);
    LONGS_EQUAL(7, SolidSyslogFacility_News);
    LONGS_EQUAL(8, SolidSyslogFacility_Uucp);
    LONGS_EQUAL(9, SolidSyslogFacility_Cron);
    LONGS_EQUAL(10, SolidSyslogFacility_AuthPriv);
    LONGS_EQUAL(11, SolidSyslogFacility_Ftp);
    LONGS_EQUAL(12, SolidSyslogFacility_Ntp);
    LONGS_EQUAL(13, SolidSyslogFacility_Audit);
    LONGS_EQUAL(14, SolidSyslogFacility_Alert);
    LONGS_EQUAL(15, SolidSyslogFacility_Clock);
    LONGS_EQUAL(16, SolidSyslogFacility_Local0);
    LONGS_EQUAL(17, SolidSyslogFacility_Local1);
    LONGS_EQUAL(18, SolidSyslogFacility_Local2);
    LONGS_EQUAL(19, SolidSyslogFacility_Local3);
    LONGS_EQUAL(20, SolidSyslogFacility_Local4);
    LONGS_EQUAL(21, SolidSyslogFacility_Local5);
    LONGS_EQUAL(22, SolidSyslogFacility_Local6);
    LONGS_EQUAL(23, SolidSyslogFacility_Local7);
}

TEST(SolidSyslogPrival, SeverityEnumValuesMatchRfc5424)
{
    LONGS_EQUAL(0, SolidSyslogSeverity_Emergency);
    LONGS_EQUAL(1, SolidSyslogSeverity_Alert);
    LONGS_EQUAL(2, SolidSyslogSeverity_Critical);
    LONGS_EQUAL(3, SolidSyslogSeverity_Error);
    LONGS_EQUAL(4, SolidSyslogSeverity_Warning);
    LONGS_EQUAL(5, SolidSyslogSeverity_Notice);
    LONGS_EQUAL(6, SolidSyslogSeverity_Informational);
    LONGS_EQUAL(7, SolidSyslogSeverity_Debug);
}
