#ifndef BDD_TARGET_BOARD_H
#define BDD_TARGET_BOARD_H

#include <stddef.h>

/* The board seam for the cross BDD targets.
 *
 * A target's main.c, the shared pipeline and the newlib retargeting are the
 * same code on every board. What they need from the board itself is declared
 * here and defined once per board, in that board's directory under
 * Bdd/Targets/Boards/; the configure chooses the directory, so the choice is
 * made at link time and nothing above this header names a board. */

/* Brings up what the rest of this header needs. Call first in main(), before
 * the scheduler starts. */
void BddTargetBoard_Init(void);

/* Writes every byte before returning. It may sleep through
 * BddTargetOsPrimitives_Sleep while the transmitter is busy, so it wants a
 * running scheduler for anything longer than the transmitter can take at
 * once. It does not serialise callers: BddTargetOsPrimitives_LockConsole is
 * what keeps one writer's bytes together. */
void BddTargetBoard_ConsoleWrite(const char* text, size_t length);

/* Returns the next byte received, sleeping through
 * BddTargetOsPrimitives_Sleep until there is one. */
char BddTargetBoard_ConsoleRead(void);

/* Ends the run and tells whatever launched it the status, where the board has
 * a way to. Does not return. */
void BddTargetBoard_Exit(int status);

#endif /* BDD_TARGET_BOARD_H */
