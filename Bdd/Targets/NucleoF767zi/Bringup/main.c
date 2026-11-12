/* Bring-up firmware for the NUCLEO-F767ZI, with no SolidSyslog in it.
 *
 * It proves the toolchain, the flash route and the serial port: a banner on
 * the ST-LINK virtual COM port, each line received echoed back, and the green
 * LED blinking so a board with nothing attached to its serial port still
 * shows it is running. */

#include "Board.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef BRINGUP_BUILD_COMMIT
#define BRINGUP_BUILD_COMMIT "unknown"
#endif

enum
{
    LINE_CAPACITY = 128,
    BLINK_HALF_PERIOD_MILLISECONDS = 500
};

static const char BANNER[] = "\r\nSolidSyslog NUCLEO-F767ZI bring-up, commit " BRINGUP_BUILD_COMMIT "\r\n";
static const char ECHO_PREFIX[] = "echo: ";
static const char LINE_END[] = "\r\n";

static char line[LINE_CAPACITY];
static size_t lineLength;
static uint32_t lastToggle;

static inline void WriteText(const char* text);
static inline void BlinkWhenDue(void);
static inline void EchoWhenLineArrives(void);
static inline bool IsLineEnd(char received);
static inline void EchoLine(void);
static inline void AppendToLine(char received);

int main(void)
{
    Board_Init();
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
    if (Board_ConsoleTryRead(&received))
    {
        if (IsLineEnd(received))
        {
            EchoLine();
        }
        else
        {
            AppendToLine(received);
        }
    }
}

static inline bool IsLineEnd(char received)
{
    return (received == '\r') || (received == '\n');
}

/* A terminal that ends a line with CR LF sends two line ends; the second
 * finds the line empty and is not echoed. */
static inline void EchoLine(void)
{
    if (lineLength > 0U)
    {
        WriteText(ECHO_PREFIX);
        Board_ConsoleWrite(line, lineLength);
        WriteText(LINE_END);
        lineLength = 0U;
    }
}

/* A line longer than the buffer is echoed truncated. */
static inline void AppendToLine(char received)
{
    if (lineLength < (size_t) LINE_CAPACITY)
    {
        line[lineLength] = received;
        lineLength++;
    }
}
