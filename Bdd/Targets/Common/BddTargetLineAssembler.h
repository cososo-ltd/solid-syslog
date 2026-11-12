#ifndef BDDTARGETLINEASSEMBLER_H
#define BDDTARGETLINEASSEMBLER_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Builds lines out of characters that arrive one at a time, for a target
       whose console is a serial port rather than a stream with a line read of
       its own. */

    enum
    {
        BDDTARGET_LINE_ASSEMBLER_CAPACITY = 128
    };

    struct BddTargetLineAssembler
    {
        /* The line, terminated, without its line end. A line longer than the
           capacity is cut to it. */
        char Text[BDDTARGET_LINE_ASSEMBLER_CAPACITY + 1];
        size_t Length;
        bool Completed;
    };

    void BddTargetLineAssembler_Reset(struct BddTargetLineAssembler * self);

    /* True when `received` ended a line, which is then in Text until the next
       character of the line after it arrives. CR and LF each end a line, and a
       line end with nothing before it is not a line, so a CR LF pair ends one
       line, not two. */
    bool BddTargetLineAssembler_Accept(struct BddTargetLineAssembler * self, char received);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETLINEASSEMBLER_H */
