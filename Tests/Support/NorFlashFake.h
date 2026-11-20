#ifndef NORFLASHFAKE_H
#define NORFLASHFAKE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogExternC.h"

struct SolidSyslogNorFlash;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* A NOR flash in memory, behind the NorFlash seam. One instance; Create
     * resets it to a blank part, every byte 0xFF. It behaves as a part does
     * where a store could tell the difference: a program clears bits and sets
     * none, and only an erase sets them. It counts what it is asked to do, and
     * it can lose power part way through a program or an erase. */

    enum
    {
        NORFLASHFAKE_MAX_BLOCK_SIZE = 4096,
        NORFLASHFAKE_MAX_BLOCK_COUNT = 64
    };

    /* A blank flash of @p blockCount blocks of @p blockSize bytes, neither more
     * than the limits above. Counts and faults are cleared. */
    struct SolidSyslogNorFlash* NorFlashFake_Create(size_t blockSize, size_t blockCount);

    /* Operations carried out since Create or the last ResetCounts. A program is
     * one call of the seam, whatever it spans. An operation that fails, or that
     * power cuts short, is not counted. */
    size_t NorFlashFake_EraseCount(void);
    size_t NorFlashFake_ProgramCount(void);
    size_t NorFlashFake_ReadCount(void);
    /* How many times @p block has been erased since Create. */
    size_t NorFlashFake_EraseCountOf(size_t block);
    void NorFlashFake_ResetCounts(void);

    /* The stored byte, and a way to set it without the rules of a program, for
     * arranging what an earlier run or a fault left on the part. */
    uint8_t NorFlashFake_Peek(size_t block, size_t offset);
    void NorFlashFake_Poke(size_t block, size_t offset, uint8_t value);

    /* The next operation of that kind answers false and does nothing. */
    void NorFlashFake_FailNextRead(void);
    void NorFlashFake_FailNextProgram(void);
    void NorFlashFake_FailNextErase(void);
    /* Every read whose span covers this byte fails, as a part that has stopped
     * answering does. */
    void NorFlashFake_FailReadsTouching(size_t block, size_t offset);

    /* The next program answers true and programs nothing, as a part does when
     * a program does not take. */
    void NorFlashFake_IgnoreNextProgram(void);

    enum NorFlashFakeCut
    {
        /* The operation does not happen at all. */
        NORFLASHFAKE_CUT_CLEAN,
        /* A program lands the first half of its bytes. An erase sets the first
         * half of the block to 0xFF and leaves the rest as it was. */
        NORFLASHFAKE_CUT_TORN
    };

    /* Power is lost during the Nth program or erase from now, counting from 1.
     * That operation answers false, and so does every operation after it,
     * reads included, until PowerOn. */
    void NorFlashFake_CutPowerAt(size_t operation, enum NorFlashFakeCut cut);
    bool NorFlashFake_PowerIsLost(void);
    /* Ends the outage, leaving the part as the cut left it. */
    void NorFlashFake_PowerOn(void);
    /* Programs and erases attempted since Create or the last PowerOn: what
     * CutPowerAt counts. */
    size_t NorFlashFake_WriteOperationCount(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* NORFLASHFAKE_H */
