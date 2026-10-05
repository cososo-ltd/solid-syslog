<#
.SYNOPSIS
Runs the BDD scenarios against the VxWorks 6.4 target under QEMU on the build
machine, from the development machine.

.DESCRIPTION
Behave runs here, natively. Before the first scenario it has the runner check
out and build the commit under test, which must therefore have been pushed;
each scenario then boots the target with the runner's qemu-start job, drives it
over the console QEMU connects back to this machine, and stops it again.

The oracle is the OpenTelemetry Collector the Windows runner uses, started here
for the run with Bdd/otel/config.vxworks64.yaml: the target's syslog crosses the
network, so it listens on every interface, for UDP and TCP alike, on the
collector port below. The target is told that port, and the address it reached
this machine at. A scenario that stops the collector to make an outage starts
it again from the same config, which the steps are told below.

Needs the job service running here (Runner\job_service.py serve), the runner
running on the build machine, Bdd/otel/bin/otelcol-contrib.exe
(Bdd/otel/Install-OtelCollector.ps1), and inbound UDP and TCP on the collector
port allowed from the local network. The collector port is 5514, the target's
own default, so nothing else may hold it: a container publishing it, such as the
devcontainer's syslog-ng, has to be stopped for the run. No other collector may
be running either, because both would write Bdd/output/received.jsonl.

The tag filter is this target's list of what it cannot do yet: the file store
(@store), TLS (@tls), and anything tagged @vxworks64wip, which a scenario
leaves as the story that gives the target the capability lands.

-SkipBuild reuses the image already built on the build machine. -Paths runs
only the features given, as behave takes them, under the same tag filter.

.EXAMPLE
.\Run-VxWorks64Bdd.ps1
.\Run-VxWorks64Bdd.ps1 -SkipBuild -Paths Bdd/features/prival.feature
#>
[CmdletBinding()]
param(
    [switch] $SkipBuild,
    [string[]] $Paths = @('Bdd/features')
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'VxWorks64Tools.ps1')

$tags = '(@udp or @tcp) and not @store and not @tls and not @wip and not @no_rtc and not @vxworks64wip'
$collectorPort = 5514
$healthUri = 'http://127.0.0.1:13133/'

if (Get-Process -Name 'otelcol-contrib' -ErrorAction SilentlyContinue)
    {
    throw 'An OpenTelemetry Collector is already running; stop it first, because both would write Bdd/output/received.jsonl.'
    }
# A container's published port is held inside the container host, where a
# probe from here cannot always see it, so ask Docker directly.
if (Get-Command docker -ErrorAction SilentlyContinue)
    {
    foreach ($protocol in @('udp', 'tcp'))
        {
        $holders = @(docker ps --filter "publish=$collectorPort/$protocol" --format '{{.Names}}' 2>$null)
        if ($holders.Count -gt 0)
            {
            throw "$($protocol.ToUpper()) $collectorPort is published by $($holders -join ', '); stop it for the run: docker stop $($holders -join ' ')"
            }
        }
    }

# The steps size their payloads from the target's build-time tunables. The
# target is built with the library's defaults, so the module is written from
# them rather than from whichever host configuration last generated it.
$defaults = Get-Content -Raw -LiteralPath (Join-Path $script:RepositoryRoot 'Core\Interface\SolidSyslogTunablesDefaults.h')
if ($defaults -notmatch '#define SOLIDSYSLOG_MAX_MESSAGE_SIZE (\d+)U')
    {
    throw 'SOLIDSYSLOG_MAX_MESSAGE_SIZE was not found in SolidSyslogTunablesDefaults.h'
    }
$template = Join-Path $script:RepositoryRoot 'Bdd\features\steps\solidsyslog_tunables.py.in'
$module = Join-Path $script:RepositoryRoot 'Bdd\features\steps\solidsyslog_tunables.py'
(Get-Content -Raw -LiteralPath $template).Replace('@SOLIDSYSLOG_BDD_MAX_MESSAGE_SIZE@', $Matches[1]) |
    Set-Content -NoNewline -Encoding utf8 -LiteralPath $module

$env:BDD_TARGET = 'vxworks64'
$env:ORACLE_FORMAT = 'otel-jsonl'
$env:RECEIVED_LOG = 'Bdd/output/received.jsonl'
$env:VXWORKS64_COLLECTOR_PORT = "$collectorPort"
$env:OTELCOL_CONFIG = 'Bdd\otel\config.vxworks64.yaml'
$env:OTELCOL_TCP_PORTS = "$collectorPort"
$env:VXWORKS64_SKIP_BUILD = if ($SkipBuild) { '1' } else { '' }

Push-Location $script:RepositoryRoot
$collector = $null
try
    {
    New-Item -ItemType Directory -Force -Path 'Bdd\output' | Out-Null
    $collector = Start-Process -FilePath 'Bdd\otel\bin\otelcol-contrib.exe' `
        -ArgumentList "--config=$env:OTELCOL_CONFIG" `
        -RedirectStandardOutput 'Bdd\output\otelcol-vxworks64.out' `
        -RedirectStandardError 'Bdd\output\otelcol-vxworks64.err' `
        -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds(30)
    $ready = $false
    while (-not $ready)
        {
        if ($collector.HasExited -or ((Get-Date) -gt $deadline))
            {
            $reason = Get-Content -LiteralPath 'Bdd\output\otelcol-vxworks64.err' -Tail 1 -ErrorAction SilentlyContinue
            throw "The OpenTelemetry Collector did not become ready: $reason"
            }
        try
            {
            $ready = (Invoke-WebRequest -UseBasicParsing -Uri $healthUri -TimeoutSec 1).StatusCode -eq 200
            }
        catch
            {
            Start-Sleep -Milliseconds 250
            }
        }

    # Behave reports progress on stderr too. Windows PowerShell turns a native
    # command's stderr into error records when its output is redirected, which
    # 'Stop' would make fatal, so the run is judged by its exit code alone.
    $ErrorActionPreference = 'Continue'
    & python -m behave --tags=$tags @Paths
    $exitCode = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    }
finally
    {
    # By name rather than by the process started above: a scenario that made an
    # outage replaced it with one of its own. A run needs sole use of the
    # collector - the outage step itself stops every collector by name, and a
    # second one could not bind the port or share the output files - so none
    # was allowed at the start, and every collector now is this run's.
    Get-Process -Name 'otelcol-contrib' -ErrorAction SilentlyContinue | Stop-Process -Force
    Pop-Location
    }
exit $exitCode
