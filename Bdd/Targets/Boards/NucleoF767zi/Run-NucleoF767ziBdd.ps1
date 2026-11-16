<#
.SYNOPSIS
Builds one of the board's BDD targets at the commit under test, flashes it to
the NUCLEO-F767ZI attached to this machine, and runs the BDD scenarios against
the board.

.DESCRIPTION
-Target is mandatory and selects the image: lwip (FreeRTOS + lwIP Raw), cmsis
(CMSIS-RTOS2 + lwIP Sockets) or plustcp (FreeRTOS-Plus-TCP). Everything below
applies to all three.

Behave runs natively on this machine. Each scenario starts the target by
resetting the board through the ST-LINK's debug probe, and drives it over the
ST-LINK's virtual COM port. Before the first scenario the run checks that the
board's banner names the image requested and the commit checked out here, so a
run cannot pass against a different image.

The image is built in the cross image CI uses, read from
.github/workflows/ci.yml, so Docker must be running. The board's addresses
depend on the network it is cabled to and are not in the repository: give them
here, or in the environment variables the parameters default to. Tracked files
must have no uncommitted changes, because the build marks such an image
"-dirty" and the run refuses it.

The oracle is the OpenTelemetry Collector the Windows runner uses, started for
the run with Bdd/otel/config.vxworks64.yaml. It listens on every interface for
UDP and TCP on the collector port. A scenario that stops the collector to
simulate an outage restarts it from the same config.

Needs STM32CubeCLT, Python with Bdd/requirements.txt and pyserial installed,
Bdd/otel/bin/otelcol-contrib.exe (Bdd/otel/Install-OtelCollector.ps1), and
inbound UDP and TCP on the collector port allowed from the board's network.
Nothing else may hold that port, and no other collector may be running,
because both would write Bdd/output/received.jsonl.

The tag filter excludes TLS (@tls), which has not been run on the board. Each
image keeps its store on a medium that survives a reset (a MicroSD card under
lwip and plustcp, a W25Q128 flash under cmsis), so the run wipes it before each
scenario that uses a store (@store). The board has a real-time clock, which the
run sets before the first scenario, so the @rtc scenarios run and the @no_rtc
scenarios do not. A scenario tagged @nucleof767ziwip is waiting on a capability
the board does not have yet.

-SkipBuild reuses the image already built, and still flashes it.
-Paths runs only the features given, in behave's syntax, under the same tag
filter.

-SustainedRun runs no scenarios. It sends the given number of messages over
UDP and then as many over TCP, and passes if the collector recorded each one
exactly once, in order, as sent, and with a timestamp no earlier than the
previous one. Runner/sustained_run.py does the work. With -FileStore every
message goes through the file store on the image's medium, which is wiped
first, at a rate the medium is known to sustain. -FileStoreInterval overrides
the image's interval, in seconds, and is how a medium's rate is found.

.EXAMPLE
.\Run-NucleoF767ziBdd.ps1 -Target lwip -BoardAddress <board address> -CollectorAddress <this machine's address>
.\Run-NucleoF767ziBdd.ps1 -Target cmsis -BoardAddress <board address> -CollectorAddress <this machine's address>
.\Run-NucleoF767ziBdd.ps1 -Target plustcp -BoardAddress <board address> -CollectorAddress <this machine's address>
.\Run-NucleoF767ziBdd.ps1 -Target lwip -SkipBuild -Paths Bdd/features/prival.feature
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('lwip', 'cmsis', 'plustcp')]
    [string] $Target,
    [string] $BoardAddress = $env:NUCLEO_F767ZI_BOARD_ADDRESS,
    [string] $CollectorAddress = $env:NUCLEO_F767ZI_COLLECTOR_ADDRESS,
    [string] $Netmask = $env:NUCLEO_F767ZI_NETMASK,
    [string] $Gateway = $env:NUCLEO_F767ZI_GATEWAY,
    [string] $SerialPort = $env:NUCLEO_F767ZI_SERIAL_PORT,
    [switch] $SkipBuild,
    [string[]] $Paths = @('Bdd/features'),
    [int] $SustainedRun = 0,
    [switch] $FileStore,
    [string] $FileStoreInterval = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..')).Path
$tags = '(@udp or @tcp) and not @tls and not @no_rtc and not @wip and not @freertoswip and not @nucleof767ziwip and not @host_store and not @host_buffer and not @passthrough'
$collectorPort = 5514
$healthUri = 'http://127.0.0.1:13133/'
# For each image: its preset, the target's directory under the build tree, the
# target name (also the image's file name), the name in its banner (the
# preset's SOLIDSYSLOG_BDD_TARGET), and the seconds a sustained run leaves
# between file-store messages. The interval is about twice what the medium
# was measured to take per message: a sector write on the card, and one or two
# erases on the flash.
$images = @{
    lwip  = @{ Preset = 'nucleo-f767zi-lwip'; Directory = 'Bdd/Targets/FreeRtosLwip'; Name = 'SolidSyslogBddTargetLwip'; Banner = 'FREERTOS_LWIP'; FileStoreInterval = '0.05' }
    cmsis = @{ Preset = 'nucleo-f767zi-cmsis-lwip'; Directory = 'Bdd/Targets/CmsisLwip'; Name = 'SolidSyslogBddTargetCmsis'; Banner = 'CMSIS_LWIP'; FileStoreInterval = '0.25' }
    plustcp = @{ Preset = 'nucleo-f767zi-plustcp'; Directory = 'Bdd/Targets/FreeRtos'; Name = 'SolidSyslogBddTarget'; Banner = 'FREERTOS_PLUSTCP'; FileStoreInterval = '0.05' }
}
$preset = $images[$Target].Preset
$buildTarget = $images[$Target].Name
$buildDirectory = "build/$preset"
$image = "$buildDirectory/$($images[$Target].Directory)/$buildTarget.elf"
# The tree's path inside the build container, which the CMake cache records.
$mount = '/workspaces/solid-syslog'
# Environment the run sets for the steps, restored when it ends. Left set,
# BDD_BOARD would send a later run of another target to this board, and the
# serial port would override detection after the board has moved ports.
$runEnvironment = @('BDD_TARGET', 'BDD_BOARD', 'NUCLEO_F767ZI_SERIAL_PORT', 'NUCLEO_F767ZI_BUILD_DIR', 'NUCLEO_F767ZI_IMAGE',
    'NUCLEO_F767ZI_CONSOLE_RECORD', 'STM32_PROGRAMMER_CLI', 'ORACLE_FORMAT', 'RECEIVED_LOG',
    'VXWORKS64_COLLECTOR_PORT', 'OTELCOL_CONFIG', 'OTELCOL_TCP_PORTS')

# STM32CubeCLT's installer puts the programmer on PATH. A shell opened before
# the install, or one that does not inherit the machine's PATH, needs the
# fallback.
function Find-Programmer
    {
    $onPath = Get-Command 'STM32_Programmer_CLI' -ErrorAction SilentlyContinue
    if ($onPath)
        {
        return $onPath.Source
        }
    $installed = @(Get-Item 'C:\ST\STM32CubeCLT_*\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe' -ErrorAction SilentlyContinue |
        Sort-Object FullName)
    if ($installed.Count -eq 0)
        {
        throw 'STM32_Programmer_CLI was not found on PATH or under C:\ST; install STM32CubeCLT.'
        }
    $installed[-1].FullName
    }

# The ST-LINK's virtual COM port, by ST's USB vendor id.
function Find-SerialPort
    {
    $found = @(& python -c "from serial.tools import list_ports; print('\n'.join(port.device for port in list_ports.comports() if port.vid == 0x0483))")
    if (($LASTEXITCODE -ne 0) -or ($found.Count -ne 1) -or (-not $found[0]))
        {
        throw "Expected one ST-LINK virtual COM port and found $($found.Count); name the board's with -SerialPort."
        }
    $found[0]
    }

function Build-Image
    {
    if (-not $BoardAddress -or -not $CollectorAddress)
        {
        throw 'A build needs -BoardAddress and -CollectorAddress, the addresses of the board and of this machine on the test network.'
        }
    $workflow = Get-Content -Raw -LiteralPath (Join-Path $repositoryRoot '.github\workflows\ci.yml')
    if ($workflow -notmatch 'ghcr\.io/cososo-ltd/cpputest-freertos-cross@sha256:[0-9a-f]{64}')
        {
        throw 'The cross image was not found in .github/workflows/ci.yml'
        }
    $crossImage = $Matches[0]
    $configure = "cmake --preset $preset -DSOLIDSYSLOG_BDD_BOARD_IP=$BoardAddress -DSOLIDSYSLOG_BDD_BOARD_ORACLE=$CollectorAddress"
    if ($Netmask)
        {
        $configure += " -DSOLIDSYSLOG_BDD_BOARD_NETMASK=$Netmask"
        }
    if ($Gateway)
        {
        $configure += " -DSOLIDSYSLOG_BDD_BOARD_GATEWAY=$Gateway"
        }
    # Runs as root with every directory trusted, as the CI lane does: the tree
    # belongs to this machine's user, and the build asks git for the commit. A
    # native command's stderr is fatal under 'Stop' when output is redirected,
    # so each is judged by its exit code alone.
    $ErrorActionPreference = 'Continue'
    & docker run --rm --user root `
        -e GIT_CONFIG_COUNT=1 -e GIT_CONFIG_KEY_0=safe.directory -e 'GIT_CONFIG_VALUE_0=*' `
        -v "${repositoryRoot}:$mount" -w $mount $crossImage `
        sh -c "$configure && cmake --build --preset $preset --target $buildTarget"
    $ErrorActionPreference = 'Stop'
    if ($LASTEXITCODE -ne 0)
        {
        throw "The build of the image failed with exit code $LASTEXITCODE."
        }
    }

if (Get-Process -Name 'otelcol-contrib' -ErrorAction SilentlyContinue)
    {
    throw 'An OpenTelemetry Collector is already running; stop it first, because both would write Bdd/output/received.jsonl.'
    }
# A container's published port is held inside the container host, where a
# probe from here cannot always see it, so Docker is asked directly.
if (Get-Command docker -ErrorAction SilentlyContinue)
    {
    foreach ($protocol in @('udp', 'tcp'))
        {
        # Docker may be installed but not running, in which case nothing of
        # its holds the port; under 'Stop' its stderr would end the run.
        $ErrorActionPreference = 'Continue'
        $holders = @(docker ps --filter "publish=$collectorPort/$protocol" --format '{{.Names}}' 2>$null)
        $ErrorActionPreference = 'Stop'
        if ($holders.Count -gt 0)
            {
            throw "$($protocol.ToUpper()) $collectorPort is published by $($holders -join ', '); stop it for the run: docker stop $($holders -join ' ')"
            }
        }
    }

Push-Location $repositoryRoot
$collector = $null
$exitCode = 1
$environmentBefore = @{}
foreach ($name in $runEnvironment)
    {
    $environmentBefore[$name] = [Environment]::GetEnvironmentVariable($name)
    }
try
    {
    if (@(& git status --porcelain --untracked-files=no).Count -gt 0)
        {
        throw 'A tracked file has changes that are not committed, so an image built here is of no commit; commit them first.'
        }
    $programmer = Find-Programmer
    if (-not $SerialPort)
        {
        $SerialPort = Find-SerialPort
        }
    if (-not $SkipBuild)
        {
        Build-Image
        }

    $ErrorActionPreference = 'Continue'
    & $programmer -c port=SWD -w $image -v -rst
    $ErrorActionPreference = 'Stop'
    if ($LASTEXITCODE -ne 0)
        {
        throw "Writing the image to the board failed with exit code $LASTEXITCODE."
        }

    # The steps size their payloads from the target's build-time tunables.
    # Every preset's configure overwrites the module they read, so it is
    # regenerated here for this build by the same rule: the library's default,
    # unless the build's user tunables file sets the value.
    $defaults = Get-Content -Raw -LiteralPath 'Core\Interface\SolidSyslogTunablesDefaults.h'
    if ($defaults -notmatch '#define\s+SOLIDSYSLOG_MAX_MESSAGE_SIZE\s+(\d+)')
        {
        throw 'SOLIDSYSLOG_MAX_MESSAGE_SIZE was not found in SolidSyslogTunablesDefaults.h'
        }
    $maxMessageSize = $Matches[1]
    $cache = Get-Content -Raw -LiteralPath "$buildDirectory/CMakeCache.txt"
    if ($cache -match '(?m)^SOLIDSYSLOG_USER_TUNABLES_FILE:\w+=(\S+)')
        {
        $userTunables = Get-Content -Raw -LiteralPath $Matches[1].Replace("$mount/", '')
        if ($userTunables -match '#define\s+SOLIDSYSLOG_MAX_MESSAGE_SIZE\s+(\d+)')
            {
            $maxMessageSize = $Matches[1]
            }
        }
    (Get-Content -Raw -LiteralPath 'Bdd\features\steps\solidsyslog_tunables.py.in').Replace('@SOLIDSYSLOG_BDD_MAX_MESSAGE_SIZE@', $maxMessageSize) |
        Set-Content -NoNewline -Encoding utf8 -LiteralPath 'Bdd\features\steps\solidsyslog_tunables.py'

    $env:BDD_TARGET = 'freertos'
    $env:BDD_BOARD = 'nucleo-f767zi'
    $env:NUCLEO_F767ZI_SERIAL_PORT = $SerialPort
    $env:NUCLEO_F767ZI_BUILD_DIR = $buildDirectory
    $env:NUCLEO_F767ZI_IMAGE = $images[$Target].Banner
    $env:STM32_PROGRAMMER_CLI = $programmer
    $env:ORACLE_FORMAT = 'otel-jsonl'
    $env:RECEIVED_LOG = 'Bdd/output/received.jsonl'
    # The collector config reads its port from this variable for every target.
    $env:VXWORKS64_COLLECTOR_PORT = "$collectorPort"
    $env:OTELCOL_CONFIG = 'Bdd\otel\config.vxworks64.yaml'
    $env:OTELCOL_TCP_PORTS = "$collectorPort"

    New-Item -ItemType Directory -Force -Path 'Bdd\output' | Out-Null
    # A timestamped record of everything that crosses the serial port, and of
    # each reset.
    $env:NUCLEO_F767ZI_CONSOLE_RECORD = Join-Path $repositoryRoot 'Bdd\output\nucleo-f767zi-console.log'
    Remove-Item -LiteralPath $env:NUCLEO_F767ZI_CONSOLE_RECORD -ErrorAction SilentlyContinue
    # Every run starts with no records. The collector rotates a record file at
    # ten megabytes, which one sustained run fills. A sustained run reads the
    # rotated files too, so none may be left from an earlier run. A scenario
    # reads only the newest file, so one left nearly full could be rotated
    # away with the message the scenario is waiting for. Finding none is not
    # an error; one that cannot be removed ends the run.
    Remove-Item -Path 'Bdd\output\received*.jsonl'
    $collector = Start-Process -FilePath 'Bdd\otel\bin\otelcol-contrib.exe' `
        -ArgumentList "--config=$env:OTELCOL_CONFIG" `
        -RedirectStandardOutput 'Bdd\output\otelcol-nucleo-f767zi.out' `
        -RedirectStandardError 'Bdd\output\otelcol-nucleo-f767zi.err' `
        -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds(30)
    $ready = $false
    while (-not $ready)
        {
        if ($collector.HasExited -or ((Get-Date) -gt $deadline))
            {
            $reason = Get-Content -LiteralPath 'Bdd\output\otelcol-nucleo-f767zi.err' -Tail 1 -ErrorAction SilentlyContinue
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

    # Behave reports progress on stderr too.
    $ErrorActionPreference = 'Continue'
    if ($SustainedRun -gt 0)
        {
        $store = if ($FileStore) { 'file' } else { 'null' }
        $interval = if ($FileStoreInterval) { $FileStoreInterval } else { $images[$Target].FileStoreInterval }
        & python (Join-Path $PSScriptRoot 'Runner\sustained_run.py') $SustainedRun $store $interval
        }
    else
        {
        & python -m behave --tags=$tags @Paths
        }
    $exitCode = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if ($SustainedRun -le 0)
        {
        Write-Host "Commit under test: $(& git rev-parse HEAD)"
        }
    }
finally
    {
    # Collectors are stopped by name, because an outage scenario replaces the
    # one started above. None was running at the start, so any running now
    # belongs to this run. Stop-Process returns before the process has gone,
    # and a run started straight afterwards would find it and refuse to start,
    # so each is waited for.
    if ($collector)
        {
        $stopped = @(Get-Process -Name 'otelcol-contrib' -ErrorAction SilentlyContinue)
        $stopped | Stop-Process -Force
        $stopped | Wait-Process -Timeout 30 -ErrorAction SilentlyContinue
        }
    foreach ($name in $runEnvironment)
        {
        [Environment]::SetEnvironmentVariable($name, $environmentBefore[$name])
        }
    Pop-Location
    }
exit $exitCode
