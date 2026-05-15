#include "BddTargetWindowsCommandLine.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

enum
{
    /* Mirrors the Linux Threaded example defaults. The block store is sized
       so a single block holds a few records, so several scenarios involving
       store rotation / discard policies trigger inside one BDD scenario. */
    DEFAULT_MAX_BLOCKS = 4,
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
    char* end = NULL;
    errno = 0;
    unsigned long value = strtoul(text, &end, 10);
    if ((end == text) || (*end != '\0') || (errno == ERANGE) || (value == 0))
    {
        return false;
    }
    *out = (size_t) value;
    return true;
}

void BddTargetWindowsCommandLine_Parse(int argc, char* argv[], struct BddTargetWindowsOptions* options)
{
    options->Facility = SolidSyslogFacility_Local0;
    options->Severity = SolidSyslogSeverity_Informational;
    options->Transport = "udp";
    options->MessageId = NULL;
    options->Msg = NULL;
    options->AppName = NULL;
    options->Store = "null";
    options->MaxBlocks = DEFAULT_MAX_BLOCKS;
    options->MaxBlockSize = DEFAULT_MAX_BLOCK_SIZE;
    options->DiscardPolicy = "oldest";
    options->CapacityThreshold = 0;
    options->HaltExit = false;
    options->NoSd = false;

    for (int i = 1; i < argc; i++)
    {
        if (((i + 1) < argc) && (strcmp(argv[i], "--facility") == 0))
        {
            options->Facility = (enum SolidSyslogFacility) atoi(argv[++i]);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--severity") == 0))
        {
            options->Severity = (enum SolidSyslogSeverity) atoi(argv[++i]);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--msgid") == 0))
        {
            options->MessageId = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--message") == 0))
        {
            options->Msg = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--transport") == 0))
        {
            options->Transport = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--app-name") == 0))
        {
            options->AppName = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--store") == 0))
        {
            options->Store = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--max-blocks") == 0))
        {
            (void) ParsePositiveSize(argv[++i], &options->MaxBlocks);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--max-block-size") == 0))
        {
            (void) ParsePositiveSize(argv[++i], &options->MaxBlockSize);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--discard-policy") == 0))
        {
            options->DiscardPolicy = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--capacity-threshold") == 0))
        {
            (void) ParsePositiveSize(argv[++i], &options->CapacityThreshold);
        }
        else if (strcmp(argv[i], "--halt-exit") == 0)
        {
            options->HaltExit = true;
        }
        else if (strcmp(argv[i], "--no-sd") == 0)
        {
            options->NoSd = true;
        }
    }
}
