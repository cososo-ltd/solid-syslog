#ifndef SOLIDSYSLOGBLOCKDEVICE_H
#define SOLIDSYSLOGBLOCKDEVICE_H

#include "ExternC.h"
#include <stdbool.h>
#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslogBlockDevice;

    /* Acquire makes a block ready for fresh writes. The contract requires the block to be empty
     * after the call (filesystem-backed drivers create-or-truncate; flash drivers erase). Must
     * not be called on a block whose existing content the integrator wants to keep — use Exists
     * + Size to detect and resume from a populated block instead.
     */
    bool   SolidSyslogBlockDevice_Acquire(struct SolidSyslogBlockDevice * device, size_t blockIndex);
    bool   SolidSyslogBlockDevice_Dispose(struct SolidSyslogBlockDevice * device, size_t blockIndex);
    bool   SolidSyslogBlockDevice_Exists(struct SolidSyslogBlockDevice * device, size_t blockIndex);
    bool   SolidSyslogBlockDevice_Read(struct SolidSyslogBlockDevice * device, size_t blockIndex, size_t offset, void* buf, size_t count);
    bool   SolidSyslogBlockDevice_Append(struct SolidSyslogBlockDevice * device, size_t blockIndex, const void* buf, size_t count);
    bool   SolidSyslogBlockDevice_WriteAt(struct SolidSyslogBlockDevice * device, size_t blockIndex, size_t offset, const void* buf, size_t count);
    size_t SolidSyslogBlockDevice_Size(struct SolidSyslogBlockDevice * device, size_t blockIndex);

EXTERN_C_END

#endif /* SOLIDSYSLOGBLOCKDEVICE_H */
