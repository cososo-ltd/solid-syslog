<#
.SYNOPSIS
Builds the FreeRTOS + lwIP Raw BDD target for the commit under test, writes it
to the NUCLEO-F767ZI attached to this machine, and runs the BDD scenarios
against the board.

.DESCRIPTION
Behave runs here, natively. Each scenario starts the target by resetting the
board through the ST-LINK's debug probe, and drives it over the ST-LINK's
virtual COM port. Before the first scenario the run checks that the banner the
board shows names the commit checked out here, so a run cannot pass against an
image of something else.

The image is built in the cross image CI builds it in, read from
.github/workflows/ci.yml, so Docker has to be running. The board's addresses
are facts about the network it is cabled to and are not in the repository: give
them here, or in the environment variables the parameters default to. The tree
must hold no change to a tracked file, because the build marks such an image
"-dirty" and the run refuses it.

The oracle is the OpenTelemetry Collector the Windows runner uses, started here
for the run with Bdd/otel/config.vxworks64.yaml, which listens on every
interface for UDP and TCP on the collector port below. A scenario that stops
the collector to make an outage starts it again from the same config, which the
steps are told below.

Needs STM32CubeCLT, Python with Bdd/requirements.txt and pyserial installed,
Bdd/otel/bin/otelcol-contrib.exe (Bdd/otel/Install-OtelCollector.ps1), and
inbound UDP and TCP on the collector port allowed from the board's network.
Nothing else may hold that port, and no other collector may be running,
because both would write Bdd/output/received.jsonl.

The tag filter is this board's list of what it cannot do yet: a store (@store),
for want of storage, and TLS (@tls), which has not been run on it. It has no
real-time clock (@rtc). Anything tagged @nucleof767ziwip is a scenario waiting
on a capability, and loses the tag as the story that adds it lands.

-SkipBuild reuses the image already built, and still writes it to the board.
-Paths runs only the features given, as behave takes them, under the same tag
filter.

.EXAMPLE
.\Run-NucleoF767ziBdd.ps1 -BoardAddress <board address> -CollectorAddress <this machine's address>
.\Run-NucleoF767ziBdd.ps1 -SkipBuild -Paths Bdd/features/prival.feature
#>
[CmdletBinding()]
param(
    [string] $BoardAddress = $env:NUCLEO_F767ZI_BOARD_ADDRESS,
    [string] $CollectorAddress = $env:NUCLEO_F767ZI_COLLECTOR_ADDRESS,
    [string] $Netmask = $env:NUCLEO_F767ZI_NETMASK,
    [string] $Gateway = $env:NUCLEO_F767ZI_GATEWAY,
    [string] $SerialPort = $env:NUCLEO_F767ZI_SERIAL_PORT,
    [switch] $SkipBuild,
    [string[]] $Paths = @('Bdd/features')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..')).Path
$tags = '(@udp or @tcp) and not @tls and not @store and not @rtc and not @wip and not @freertoswip and not @nucleof767ziwip and not @host_store and not @host_buffer and not @passthrough'
$collectorPort = 5514
$healthUri = 'http://127.0.0.1:13133/'
$preset = 'nucleo-f767zi-lwip'
$buildDirectory = "build/$preset"
$image = "$buildDirectory/Bdd/Targets/FreeRtosLwip/SolidSyslogBddTargetLwip.elf"

# STM32CubeCLT's installer puts the programmer on PATH; a shell opened before
# the install, or one that does not take the machine's PATH, finds it here.
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
    # As root, and with every directory trusted, as the CI lane runs it: the
    # tree is this machine's, and the build asks git which commit it is. A
    # native command's stderr is fatal under 'Stop' when output is redirected,
    # so each is judged by its exit code alone.
    $ErrorActionPreference = 'Continue'
    & docker run --rm --user root `
        -e GIT_CONFIG_COUNT=1 -e GIT_CONFIG_KEY_0=safe.directory -e 'GIT_CONFIG_VALUE_0=*' `
        -v "${repositoryRoot}:/workspaces/solid-syslog" -w /workspaces/solid-syslog $crossImage `
        sh -c "$configure && cmake --build --preset $preset --target SolidSyslogBddTargetLwip"
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

Push-Location $repositoryRoot
$collector = $null
$exitCode = 1
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

    # The steps size their payloads from the target's build-time tunables. The
    # target is built with the library's default message size, so the module is
    # written from it rather than from whichever host configuration last
    # generated it.
    $defaults = Get-Content -Raw -LiteralPath 'Core\Interface\SolidSyslogTunablesDefaults.h'
    if ($defaults -notmatch '#define SOLIDSYSLOG_MAX_MESSAGE_SIZE (\d+)U')
        {
        throw 'SOLIDSYSLOG_MAX_MESSAGE_SIZE was not found in SolidSyslogTunablesDefaults.h'
        }
    (Get-Content -Raw -LiteralPath 'Bdd\features\steps\solidsyslog_tunables.py.in').Replace('@SOLIDSYSLOG_BDD_MAX_MESSAGE_SIZE@', $Matches[1]) |
        Set-Content -NoNewline -Encoding utf8 -LiteralPath 'Bdd\features\steps\solidsyslog_tunables.py'

    $env:BDD_TARGET = 'freertos'
    $env:BDD_BOARD = 'nucleo-f767zi'
    $env:NUCLEO_F767ZI_SERIAL_PORT = $SerialPort
    $env:NUCLEO_F767ZI_BUILD_DIR = $buildDirectory
    $env:STM32_PROGRAMMER_CLI = $programmer
    $env:ORACLE_FORMAT = 'otel-jsonl'
    $env:RECEIVED_LOG = 'Bdd/output/received.jsonl'
    # The collector config takes its port from this variable, whichever target
    # the run is against.
    $env:VXWORKS64_COLLECTOR_PORT = "$collectorPort"
    $env:OTELCOL_CONFIG = 'Bdd\otel\config.vxworks64.yaml'
    $env:OTELCOL_TCP_PORTS = "$collectorPort"

    New-Item -ItemType Directory -Force -Path 'Bdd\output' | Out-Null
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
    & python -m behave --tags=$tags @Paths
    $exitCode = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    Write-Host "Commit under test: $(& git rev-parse HEAD)"
    }
finally
    {
    # By name: an outage scenario replaces the collector started above. No
    # collector was running at the start, so every one now is this run's.
    if ($collector)
        {
        Get-Process -Name 'otelcol-contrib' -ErrorAction SilentlyContinue | Stop-Process -Force
        }
    Pop-Location
    }
exit $exitCode
