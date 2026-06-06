#include "BddTargetInteractive.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BddTargetCustomSd.h"
#include "SolidSyslog.h"
#include "SolidSyslogTunables.h"

static const char* const PROMPT = "SolidSyslog> ";

enum
{
    /* Sized to SOLIDSYSLOG_MAX_MESSAGE_SIZE so a single `set msg <body>`
     * can carry a full path-MTU-class message body without fgets
     * splitting it across reads. The HandleSet name[] mirrors this size
     * because the parser splits at the first whitespace; future work
     * may decouple the name buffer (always short — RFC 5424 maxima are
     * ≤ 255 chars) from the line buffer. */
    MAX_LINE_LENGTH = SOLIDSYSLOG_MAX_MESSAGE_SIZE
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

static void HandleSend(struct SolidSyslog* handle, const char* args, const struct SolidSyslogMessage* message)
{
    int count = ParseCount(args);

    for (int i = 0; i < count; i++)
    {
        SolidSyslog_Log(handle, message);
    }

    printf("Sent %d message%s\n", count, (count == 1) ? "" : "s");
}

static void HandleSendCustom(struct SolidSyslog* handle, const char* args, const struct SolidSyslogMessage* message)
{
    int count = ParseCount(args);
    struct SolidSyslogStructuredData* sd[] = {BddTargetCustomSd_Get()};

    for (int i = 0; i < count; i++)
    {
        SolidSyslog_LogWithSd(handle, message, sd, 1);
    }

    printf("Sent %d custom message%s\n", count, (count == 1) ? "" : "s");
}

static void HandleSet(const char* args, BddTargetInteractiveSetHandler onSet)
{
    char name[MAX_LINE_LENGTH];
    const char* space = strchr(args, ' ');
    size_t nameLen = (space != NULL) ? (size_t) (space - args) : strlen(args);
    const char* value = (space != NULL) ? (space + 1) : "";

    memcpy(name, args, nameLen);
    name[nameLen] = '\0';

    if (onSet(name, value))
    {
        printf("set %s=%s\n", name, value);
    }
    else
    {
        printf("set: invalid\n");
    }
}

void BddTargetInteractive_Run(
    struct SolidSyslog* handle,
    const struct SolidSyslogMessage* message,
    FILE* input,
    BddTargetInteractiveSwitchHandler onSwitch,
    BddTargetInteractiveSetHandler onSet
)
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
        if (MatchCommand(line, "send-custom", &args))
        {
            HandleSendCustom(handle, args, message);
        }
        else if (MatchCommand(line, "send", &args))
        {
            HandleSend(handle, args, message);
        }
        else if (onSwitch != NULL && MatchCommand(line, "switch", &args))
        {
            onSwitch(args);
        }
        else if (onSet != NULL && MatchCommand(line, "set", &args))
        {
            HandleSet(args, onSet);
        }

        PrintPrompt();
    }
}
