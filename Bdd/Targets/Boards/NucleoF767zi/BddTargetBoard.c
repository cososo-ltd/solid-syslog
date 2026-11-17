/* ST's NUCLEO-F767ZI behind the board seam - see BddTargetBoard.h.
 *
 * The console is USART3, which the on-board ST-LINK carries as its virtual COM
 * port. The board has nowhere to report an exit status, so a run ends with an
 * exit line on the console, carrying the status if it failed, and a halt. */

#include "BddTargetBoard.h"

#include "BddTargetOsPrimitives.h"
#include "Board.h"
#include "BoardClock.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef BDD_TARGET_BOARD_BUILD_COMMIT
#define BDD_TARGET_BOARD_BUILD_COMMIT "unknown"
#endif

/* The build's SOLIDSYSLOG_BDD_TARGET. Every image built from a commit names
 * that commit, so the banner names the image as well. */
#ifndef BDD_TARGET_BOARD_IMAGE
#define BDD_TARGET_BOARD_IMAGE "unknown"
#endif

enum
{
    CONSOLE_POLL_MILLISECONDS = 1,
    EXIT_TEXT_BYTES = 32
};

static const char BANNER[] = "\r\nSolidSyslog BDD target on NUCLEO-F767ZI, image " BDD_TARGET_BOARD_IMAGE
                             ", commit " BDD_TARGET_BOARD_BUILD_COMMIT "\r\n";
static const char EXIT_OK[] = "\r\nexit: ok\r\n";
static const char EXIT_FAILED_FORMAT[] = "\r\nexit: failed %d\r\n";

void BddTargetBoard_Init(void)
{
    Board_Init();
    Board_ConsoleWrite(BANNER, strlen(BANNER));
    /* After the banner, which a run waits for: from cold this waits for the
     * clock's crystal. */
    BoardClock_Start();
}

void BddTargetBoard_ConsoleWrite(const char* text, size_t length)
{
    Board_ConsoleWrite(text, length);
}

/* Bytes are received under interrupt and wait in the board's ring, so the
 * sleep between polls costs only latency. */
char BddTargetBoard_ConsoleRead(void)
{
    char received = '\0';
    while (!Board_ConsoleTryRead(&received))
    {
        BddTargetOsPrimitives_Sleep(CONSOLE_POLL_MILLISECONDS);
    }
    return received;
}

/* The red LED is lit for a failure, so it shows with nothing reading the
 * console. A reset, from the probe or the black button, starts the next run. */
void BddTargetBoard_Exit(int status)
{
    bool failed = status != 0;
    char text[EXIT_TEXT_BYTES] = {0};
    int formatted =
        failed ? snprintf(text, sizeof(text), EXIT_FAILED_FORMAT, status) : snprintf(text, sizeof(text), "%s", EXIT_OK);
    if ((formatted > 0) && ((size_t) formatted < sizeof(text)))
    {
        Board_ConsoleWrite(text, (size_t) formatted);
    }
    Board_Halt(failed);
}

void BddTargetBoard_Fault(const char* what, const char* where)
{
    Board_Fault(what, where);
}
