#ifndef SOLIDSYSLOG_TUNABLES_H
#define SOLIDSYSLOG_TUNABLES_H

#if defined(SOLIDSYSLOG_USER_TUNABLES_FILE)
/* cppcheck-suppress preprocessorErrorDirective -- macro expands to a header path supplied via -D at build time; the #if guard makes it active only then. */
#include SOLIDSYSLOG_USER_TUNABLES_FILE
#endif
#include "SolidSyslogTunablesDefaults.h" // IWYU pragma: export

#endif /* SOLIDSYSLOG_TUNABLES_H */
