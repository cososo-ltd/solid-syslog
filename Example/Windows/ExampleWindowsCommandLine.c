#include "ExampleWindowsCommandLine.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

enum
{
    /* Mirrors the Linux Threaded example defaults. The block store is sized
       so a single block holds a few records, so several scenarios involving
       store rotation / discard policies trigger inside one BDD scenario. */
    DEFAULT_MAX_BLOCKS     = 4,
    DEFAULT_MAX_BLOCK_SIZE = 1024
};

static bool ParsePositiveSize(const char* text, size_t* out)
{
    if ((text == NULL) || (text[0] == '\0'))
    {
        return false;
    }
    /* Reject leading sign — strtoul silently wraps "-1" to ULONG_MAX, which
       would let "--max-blocks -1" produce a huge size_t. */
    if ((text[0] == '-') || (text[0] == '+'))
    {
        return false;
    }
    char* end           = NULL;
    errno               = 0;
    unsigned long value = strtoul(text, &end, 10);
    if ((end == text) || (*end != '\0') || (errno == ERANGE) || (value == 0))
    {
        return false;
    }
    *out = (size_t) value;
    return true;
}

void ExampleWindowsCommandLine_Parse(int argc, char* argv[], struct WindowsExampleOptions* options)
{
    options->facility          = SOLIDSYSLOG_FACILITY_LOCAL0;
    options->severity          = SOLIDSYSLOG_SEVERITY_INFO;
    options->transport         = "udp";
    options->messageId         = NULL;
    options->msg               = NULL;
    options->appName           = NULL;
    options->store             = "null";
    options->maxBlocks         = DEFAULT_MAX_BLOCKS;
    options->maxBlockSize      = DEFAULT_MAX_BLOCK_SIZE;
    options->discardPolicy     = "oldest";
    options->capacityThreshold = 0;
    options->haltExit          = false;
    options->noSd              = false;

    for (int i = 1; i < argc; i++)
    {
        if (((i + 1) < argc) && (strcmp(argv[i], "--facility") == 0))
        {
            options->facility = (enum SolidSyslog_Facility) atoi(argv[++i]);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--severity") == 0))
        {
            options->severity = (enum SolidSyslog_Severity) atoi(argv[++i]);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--msgid") == 0))
        {
            options->messageId = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--message") == 0))
        {
            options->msg = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--transport") == 0))
        {
            options->transport = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--app-name") == 0))
        {
            options->appName = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--store") == 0))
        {
            options->store = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--max-blocks") == 0))
        {
            (void) ParsePositiveSize(argv[++i], &options->maxBlocks);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--max-block-size") == 0))
        {
            (void) ParsePositiveSize(argv[++i], &options->maxBlockSize);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--discard-policy") == 0))
        {
            options->discardPolicy = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--capacity-threshold") == 0))
        {
            (void) ParsePositiveSize(argv[++i], &options->capacityThreshold);
        }
        else if (strcmp(argv[i], "--halt-exit") == 0)
        {
            options->haltExit = true;
        }
        else if (strcmp(argv[i], "--no-sd") == 0)
        {
            options->noSd = true;
        }
    }
}
