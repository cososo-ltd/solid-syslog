#include "BddTargetCommandLine.h"

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum
{
    DEFAULT_MAX_BLOCKS = 10,
    DEFAULT_MAX_BLOCK_SIZE = 65536,
    OPT_MAX_BLOCKS = 256,
    OPT_MAX_BLOCK_SIZE = 257,
    OPT_DISCARD_POLICY = 258,
    OPT_NO_SD = 259,
    OPT_HALT_EXIT = 260,
    OPT_CAPACITY_THRESHOLD = 261,
    OPT_APP_NAME = 262
};

static bool ParsePositiveNumber(const char* str, size_t* result)
{
    char* end = NULL;
    long value = strtol(str, &end, 10);

    if ((*str == '\0') || (*end != '\0') || (value < 0))
    {
        return false;
    }

    *result = (size_t) value;
    return true;
}

static bool IsValidDiscardPolicy(const char* policy)
{
    return (strcmp(policy, "oldest") == 0) || (strcmp(policy, "newest") == 0) || (strcmp(policy, "halt") == 0);
}

int BddTargetCommandLine_Parse(int argc, char* argv[], struct BddTargetOptions* options)
{
    options->facility = SolidSyslogFacility_Local0;
    options->severity = SolidSyslogSeverity_Informational;
    options->messageId = NULL;
    options->msg = NULL;
    options->appName = NULL;
    options->transport = "udp";
    options->store = "null";
    options->maxBlocks = DEFAULT_MAX_BLOCKS;
    options->maxBlockSize = DEFAULT_MAX_BLOCK_SIZE;
    options->discardPolicy = "oldest";
    options->capacityThreshold = 0;
    options->noSd = false;
    options->haltExit = false;

    static struct option longOptions[] = {
        {"facility", required_argument, NULL, 'f'},
        {"severity", required_argument, NULL, 's'},
        {"msgid", required_argument, NULL, 'i'},
        {"message", required_argument, NULL, 'm'},
        {"transport", required_argument, NULL, 't'},
        {"store", required_argument, NULL, 'o'},
        {"max-blocks", required_argument, NULL, OPT_MAX_BLOCKS},
        {"max-block-size", required_argument, NULL, OPT_MAX_BLOCK_SIZE},
        {"discard-policy", required_argument, NULL, OPT_DISCARD_POLICY},
        {"capacity-threshold", required_argument, NULL, OPT_CAPACITY_THRESHOLD},
        {"no-sd", no_argument, NULL, OPT_NO_SD},
        {"halt-exit", no_argument, NULL, OPT_HALT_EXIT},
        {"app-name", required_argument, NULL, OPT_APP_NAME},
        {NULL, 0, NULL, 0},
    };

    int opt = 0;
    while ((opt = getopt_long(argc, argv, "f:s:i:m:t:o:", longOptions, NULL)) != -1)
    {
        switch (opt)
        {
            case 'f':
                options->facility = (enum SolidSyslogFacility) atoi(optarg);
                break;
            case 's':
                options->severity = (enum SolidSyslogSeverity) atoi(optarg);
                break;
            case 'i':
                options->messageId = optarg;
                break;
            case 'm':
                options->msg = optarg;
                break;
            case 't':
                if ((strcmp(optarg, "udp") != 0) && (strcmp(optarg, "tcp") != 0) && (strcmp(optarg, "tls") != 0) &&
                    (strcmp(optarg, "mtls") != 0))
                {
                    return 1;
                }
                options->transport = optarg;
                break;
            case 'o':
                if ((strcmp(optarg, "null") != 0) && (strcmp(optarg, "file") != 0))
                {
                    return 1;
                }
                options->store = optarg;
                break;
            case OPT_MAX_BLOCKS:
                if (!ParsePositiveNumber(optarg, &options->maxBlocks))
                {
                    return 1;
                }
                break;
            case OPT_MAX_BLOCK_SIZE:
                if (!ParsePositiveNumber(optarg, &options->maxBlockSize))
                {
                    return 1;
                }
                break;
            case OPT_DISCARD_POLICY:
                if (!IsValidDiscardPolicy(optarg))
                {
                    return 1;
                }
                options->discardPolicy = optarg;
                break;
            case OPT_CAPACITY_THRESHOLD:
                if (!ParsePositiveNumber(optarg, &options->capacityThreshold))
                {
                    return 1;
                }
                break;
            case OPT_NO_SD:
                options->noSd = true;
                break;
            case OPT_HALT_EXIT:
                options->haltExit = true;
                break;
            case OPT_APP_NAME:
                options->appName = optarg;
                break;
            default:
                return 1;
        }
    }

    return 0;
}
