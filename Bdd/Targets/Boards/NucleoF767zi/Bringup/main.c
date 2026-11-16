/* Bring-up firmware for the NUCLEO-F767ZI, with no SolidSyslog in it.
 *
 * It proves the toolchain, the flash route and the serial port: a banner on
 * the ST-LINK virtual COM port, each line received echoed back, and the green
 * LED blinking so a board with nothing attached to its serial port still
 * shows it is running. The line "probe" is echoed like any other, and then
 * the two storage breakouts are asked whether they are there. The line
 * "flash" has the W25Q128 erased, programmed and read back. */

#include "BddTargetLineAssembler.h"
#include "Board.h"
#include "FlashCheck.h"
#include "StorageProbe.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifndef BDD_TARGET_BOARD_BUILD_COMMIT
#define BDD_TARGET_BOARD_BUILD_COMMIT "unknown"
#endif

enum
{
    BLINK_HALF_PERIOD_MILLISECONDS = 500
};

static const char BANNER[] = "\r\nSolidSyslog NUCLEO-F767ZI bring-up, commit " BDD_TARGET_BOARD_BUILD_COMMIT "\r\n";
static const char ECHO_PREFIX[] = "echo: ";
static const char LINE_END[] = "\r\n";
static const char PROBE_COMMAND[] = "probe";
static const char FLASH_COMMAND[] = "flash";

static struct BddTargetLineAssembler line;
static uint32_t lastToggle;

static inline void WriteText(const char* text);
static inline void BlinkWhenDue(void);
static inline void EchoWhenLineArrives(void);
static inline bool IsCommand(const char* command);

int main(void)
{
    Board_Init();
    StorageProbe_Init();
    BddTargetLineAssembler_Reset(&line);
    WriteText(BANNER);
    for (;;)
    {
        BlinkWhenDue();
        EchoWhenLineArrives();
    }
}

static inline void WriteText(const char* text)
{
    Board_ConsoleWrite(text, strlen(text));
}

static inline void BlinkWhenDue(void)
{
    uint32_t now = Board_Milliseconds();
    if ((now - lastToggle) >= (uint32_t) BLINK_HALF_PERIOD_MILLISECONDS)
    {
        lastToggle = now;
        Board_LedToggle(BOARD_LED_GREEN);
    }
}

static inline void EchoWhenLineArrives(void)
{
    char received = '\0';
    if (Board_ConsoleTryRead(&received) && BddTargetLineAssembler_Accept(&line, received))
    {
        WriteText(ECHO_PREFIX);
        Board_ConsoleWrite(line.Text, line.Length);
        WriteText(LINE_END);
        if (IsCommand(PROBE_COMMAND))
        {
            StorageProbe_Run();
        }
        else if (IsCommand(FLASH_COMMAND))
        {
            FlashCheck_Run();
        }
        else
        {
            /* A line to echo and no more. */
        }
    }
}

static inline bool IsCommand(const char* command)
{
    return (line.Length == strlen(command)) && (memcmp(line.Text, command, line.Length) == 0);
}
