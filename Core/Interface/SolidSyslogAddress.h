#ifndef SOLIDSYSLOGADDRESS_H
#define SOLIDSYSLOGADDRESS_H

#include "ExternC.h"
#include <stdint.h>

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_ADDRESS_SIZE = 128 /* bytes; sized to cover struct sockaddr_storage for cross-platform parity (glibc, Winsock) */
    };

    typedef struct
    {
        intptr_t slots[SOLIDSYSLOG_ADDRESS_SIZE / sizeof(intptr_t)];
    } SolidSyslogAddressStorage;

    struct SolidSyslogAddress;

    struct SolidSyslogAddress* SolidSyslogAddress_FromStorage(SolidSyslogAddressStorage * storage);

EXTERN_C_END

#endif /* SOLIDSYSLOGADDRESS_H */
