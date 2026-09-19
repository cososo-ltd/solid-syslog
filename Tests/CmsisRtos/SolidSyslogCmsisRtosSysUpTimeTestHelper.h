#ifndef SOLIDSYSLOGCMSISRTOSSYSUPTIMETESTHELPER_H
#define SOLIDSYSLOGCMSISRTOSSYSUPTIMETESTHELPER_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The adapter counts the kernel tick counter's wraps in file-scope state
       that nothing public can clear, and it has to: a callback the integrator
       points at has no Create to hang a handle off. Tests need each case to
       start from a known phase, so this clears it. */
    void TestCmsisRtosSysUpTime_Reset(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGCMSISRTOSSYSUPTIMETESTHELPER_H */
