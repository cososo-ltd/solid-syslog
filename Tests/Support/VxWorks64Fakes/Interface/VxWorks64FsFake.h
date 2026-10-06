#ifndef VXWORKS64FSFAKE_H
#define VXWORKS64FSFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Controls the file-system calls the stand-in sys/stat.h, dosFsLib.h and
     * hrfsLib.h rename, so a test never includes those headers. The volume starts blank,
     * as the file system monitor leaves an unformatted disk: rawFs, which stat
     * cannot answer for. */
    void VxWorks64FsFake_Reset(void);

    /** Make the volume dosFs, as a successful dosFsVolFormat does. */
    void VxWorks64FsFake_FormatTheVolumeAsDosFs(void);

    /** Make the volume HRFS, as a successful hrfsFormat does. */
    void VxWorks64FsFake_FormatTheVolumeAsHrfs(void);

    /** Make stat answer OK for the root, but as a regular file. */
    void VxWorks64FsFake_MakeTheRootAFile(void);

    /** Make both formatters answer ERROR and leave the volume blank. */
    void VxWorks64FsFake_FailFormats(void);

    unsigned VxWorks64FsFake_StatCallCount(void);

    const char* VxWorks64FsFake_LastStatName(void);

    unsigned VxWorks64FsFake_DosFsVolFormatCallCount(void);

    unsigned VxWorks64FsFake_HrfsFormatCallCount(void);

    unsigned VxWorks64FsFake_DosFsVolDescGetCallCount(void);

    const char* VxWorks64FsFake_LastVolDescGetName(void);

    /** Whether the last dosFsVolDescGet asked for no path tail back. */
    int VxWorks64FsFake_LastVolDescGetHadNoTail(void);

    /** Whether the last hrfsFormat left the disk size, block size and inode
     *  count to the formatter, as zero for each does. */
    int VxWorks64FsFake_LastHrfsFormatTookTheDefaults(void);

    /** The path the last format of either kind was given. */
    const char* VxWorks64FsFake_LastFormatPath(void);

    int VxWorks64FsFake_LastFormatOptions(void);

    int VxWorks64FsFake_LastFormatHadNoPrompt(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* VXWORKS64FSFAKE_H */
