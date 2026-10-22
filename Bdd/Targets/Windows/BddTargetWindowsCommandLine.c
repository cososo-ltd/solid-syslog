#include "BddTargetWindowsCommandLine.h"

#include "BddTargetStoreSettings.h"

#include <stdlib.h>
#include <string.h>

void BddTargetWindowsCommandLine_Parse(int argc, char* argv[], struct BddTargetWindowsOptions* options)
{
    options->Facility = SOLIDSYSLOG_FACILITY_LOCAL0;
    options->Severity = SOLIDSYSLOG_SEVERITY_INFORMATIONAL;
    options->Transport = "udp";
    options->MessageId = NULL;
    options->Msg = NULL;
    options->AppName = NULL;
    options->Store = "null";
    options->ErrorsFatal = "1";
    BddTargetStoreSettings_Reset();

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
        else if (((i + 1) < argc) && (strcmp(argv[i], "--errors-fatal") == 0))
        {
            options->ErrorsFatal = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--store") == 0))
        {
            options->Store = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--max-blocks") == 0))
        {
            (void) BddTargetStoreSettings_SetByName("max-blocks", argv[++i]);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--max-block-size") == 0))
        {
            (void) BddTargetStoreSettings_SetByName("max-block-size", argv[++i]);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--discard-policy") == 0))
        {
            (void) BddTargetStoreSettings_SetByName("discard-policy", argv[++i]);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--security-policy") == 0))
        {
            (void) BddTargetStoreSettings_SetByName("security-policy", argv[++i]);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--capacity-threshold") == 0))
        {
            (void) BddTargetStoreSettings_SetByName("capacity-threshold", argv[++i]);
        }
        else if (strcmp(argv[i], "--halt-exit") == 0)
        {
            (void) BddTargetStoreSettings_SetByName("halt-exit", "1");
        }
        else if (strcmp(argv[i], "--no-sd") == 0)
        {
            (void) BddTargetStoreSettings_SetByName("no-sd", "1");
        }
    }
}
