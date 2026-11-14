#ifndef BOARD_CLOCK_H
#define BOARD_CLOCK_H

/* Starts the clock behind BddTargetBoardClock.h, which is defined beside this
 * header. A board that was set before a reset knows the time as this returns;
 * one powered up since does not until it is told. From cold it waits a second
 * or two for the clock's crystal. Call once, after Board_Init and before the
 * scheduler starts. */
void BoardClock_Start(void);

#endif
