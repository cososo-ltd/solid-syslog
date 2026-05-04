#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogBuffer.h"

void SolidSyslogBuffer_Write(struct SolidSyslogBuffer* buffer, const void* data, size_t size)
{
    buffer->Write(buffer, data, size);
}

bool SolidSyslogBuffer_Read(struct SolidSyslogBuffer* buffer, void* data, size_t maxSize, size_t* bytesRead)
{
    return buffer->Read(buffer, data, maxSize, bytesRead);
}
