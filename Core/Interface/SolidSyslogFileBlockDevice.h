#ifndef SOLIDSYSLOGFILEBLOCKDEVICE_H
#define SOLIDSYSLOGFILEBLOCKDEVICE_H

#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogBlockDevice;
struct SolidSyslogFile;

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_FILEBLOCKDEVICE_STORAGE_SIZE = sizeof(intptr_t) * 16U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_FILEBLOCKDEVICE_STORAGE_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogFileBlockDeviceStorage;

    /* The driver caches at most one open SolidSyslogFile handle, re-pointed only when the
     * targeted blockIndex changes — same-block runs of Read and Append (and Append-then-WriteAt
     * during MarkSent) share the handle. The single-handle-per-path invariant the storage layer
     * depends on (E27 #345 / S27.01) is enforced by construction here: the driver physically
     * holds one file. */
    struct SolidSyslogBlockDevice* SolidSyslogFileBlockDevice_Create(
        SolidSyslogFileBlockDeviceStorage * storage,
        struct SolidSyslogFile * file,
        const char* pathPrefix
    );
    void SolidSyslogFileBlockDevice_Destroy(struct SolidSyslogBlockDevice * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGFILEBLOCKDEVICE_H */
