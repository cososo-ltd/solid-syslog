#ifndef BOARD_CLOCK_H
#define BOARD_CLOCK_H

/* Starts the clock behind BddTargetBoardClock.h. A board whose clock was set
 * before a reset has the time when this returns; one powered up since has
 * none until it is set. From cold it waits a second or two for the crystal.
 * Call once, after Board_Init and before the scheduler starts. */
void BoardClock_Start(void);

#endif
