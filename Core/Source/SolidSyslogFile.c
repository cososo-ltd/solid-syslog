#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogFile.h"

bool SolidSyslogFile_Open(struct SolidSyslogFile* file, const char* path)
{
    return file->Open(file, path);
}

void SolidSyslogFile_Close(struct SolidSyslogFile* file)
{
    file->Close(file);
}

bool SolidSyslogFile_IsOpen(struct SolidSyslogFile* file)
{
    return file->IsOpen(file);
}

bool SolidSyslogFile_Read(struct SolidSyslogFile* file, void* buf, size_t count)
{
    return file->Read(file, buf, count);
}

bool SolidSyslogFile_Write(struct SolidSyslogFile* file, const void* buf, size_t count)
{
    return file->Write(file, buf, count);
}

void SolidSyslogFile_SeekTo(struct SolidSyslogFile* file, size_t offset)
{
    file->SeekTo(file, offset);
}

size_t SolidSyslogFile_Size(struct SolidSyslogFile* file)
{
    return file->Size(file);
}

void SolidSyslogFile_Truncate(struct SolidSyslogFile* file)
{
    file->Truncate(file);
}

bool SolidSyslogFile_Exists(struct SolidSyslogFile* file, const char* path)
{
    return file->Exists(file, path);
}

bool SolidSyslogFile_Delete(struct SolidSyslogFile* file, const char* path)
{
    return file->Delete(file, path);
}
