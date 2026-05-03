#ifndef SOLIDSYSLOGFILEBLOCKDEVICE_H
#define SOLIDSYSLOGFILEBLOCKDEVICE_H

#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogFile.h"

#include <stdint.h>

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_FILEBLOCKDEVICE_STORAGE_SIZE = sizeof(intptr_t) * 16
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_FILEBLOCKDEVICE_STORAGE_SIZE + sizeof(intptr_t) - 1) / sizeof(intptr_t)];
    } SolidSyslogFileBlockDeviceStorage;

    struct SolidSyslogBlockDevice* SolidSyslogFileBlockDevice_Create(SolidSyslogFileBlockDeviceStorage * storage, struct SolidSyslogFile * readFile,
                                                                     struct SolidSyslogFile * writeFile, const char* pathPrefix);
    void                           SolidSyslogFileBlockDevice_Destroy(struct SolidSyslogBlockDevice * device);

EXTERN_C_END

#endif /* SOLIDSYSLOGFILEBLOCKDEVICE_H */
