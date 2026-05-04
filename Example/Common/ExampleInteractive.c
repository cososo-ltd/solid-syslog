#include "ExampleInteractive.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SolidSyslog.h"

static const char* const PROMPT = "SolidSyslog> ";

enum
{
    MAX_LINE_LENGTH = 256
};

static void PrintPrompt(void)
{
    printf("%s", PROMPT);
    fflush(stdout);
}

static bool MatchCommand(const char* line, const char* command, const char** args)
{
    size_t length = strlen(command);

    if (strncmp(line, command, length) != 0)
    {
        return false;
    }

    if (line[length] != ' ' && line[length] != '\0')
    {
        return false;
    }

    const char* rest = line + length;
    if (*rest == ' ')
    {
        rest++;
    }

    *args = rest;
    return true;
}

static int ParseCount(const char* args)
{
    if (args[0] == '\0')
    {
        return 1;
    }

    int count = atoi(args);
    return (count > 0) ? count : 1;
}

static void HandleSend(const char* args, const struct SolidSyslogMessage* message)
{
    int count = ParseCount(args);

    for (int i = 0; i < count; i++)
    {
        SolidSyslog_Log(message);
    }

    printf("Sent %d message%s\n", count, (count == 1) ? "" : "s");
}

void ExampleInteractive_Run(const struct SolidSyslogMessage* message, FILE* input, ExampleInteractiveSwitchHandler onSwitch)
{
    char line[MAX_LINE_LENGTH];

    PrintPrompt();

    while (fgets(line, sizeof(line), input) != NULL)
    {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
        {
            line[len - 1] = '\0';
        }

        if (strcmp(line, "quit") == 0)
        {
            break;
        }

        const char* args = NULL;
        if (MatchCommand(line, "send", &args))
        {
            HandleSend(args, message);
        }
        else if (onSwitch != NULL && MatchCommand(line, "switch", &args))
        {
            onSwitch(args);
        }

        PrintPrompt();
    }
}
