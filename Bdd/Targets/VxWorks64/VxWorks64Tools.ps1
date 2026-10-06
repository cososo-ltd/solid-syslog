# Helpers shared by the VxWorks 6.4 recipe scripts. Dot-source, do not run.
# Written for Windows PowerShell 5.1.

Set-StrictMode -Version Latest

$script:RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path

function Get-DefaultProjectDirectory
    {
    param([Parameter(Mandatory)] [string] $Tool)

    Join-Path $script:RepositoryRoot "build\vxworks64\$Tool"
    }

# The disk image QEMU gives the target as its IDE disk, where the file store
# lives. Outside the projects, so rebuilding one leaves it alone; the runner's
# store-reset job removes it, and the next QEMU start creates it blank.
function Get-StoreDiskPath
    {
    Join-Path $script:RepositoryRoot 'build\vxworks64\store-disk.img'
    }

function Get-ProjectFile
    {
    param([Parameter(Mandatory)] [string] $ProjectDirectory)

    Join-Path $ProjectDirectory 'SolidSyslogVxWorks64.wpj'
    }

# Where the project's copy of libsolidsyslog.a is built.
function Get-LibraryDirectory
    {
    param([Parameter(Mandatory)] [string] $ProjectDirectory)

    Join-Path $ProjectDirectory 'solidsyslog'
    }

function Get-BddTargetDirectory
    {
    param([Parameter(Mandatory)] [string] $ProjectDirectory)

    Join-Path $ProjectDirectory 'solidsyslogbdd'
    }

function ConvertTo-MakePath
    {
    param([Parameter(Mandatory)] [string] $Path)

    $Path.Replace('\', '/')
    }

# The Wind River make rules pass paths through unquoted command lines.
function Assert-NoWhitespace
    {
    param(
        [Parameter(Mandatory)] [string] $Path,
        [Parameter(Mandatory)] [string] $Description
    )

    if ($Path -match '\s')
        {
        throw "$Description must not contain whitespace: $Path"
        }
    }

function Get-WrenvPath
    {
    param([Parameter(Mandatory)] [string] $WindRiverRoot)

    $wrenv = Join-Path $WindRiverRoot 'wrenv.exe'
    if (-not (Test-Path -LiteralPath $wrenv -PathType Leaf))
        {
        throw "wrenv.exe not found under $WindRiverRoot - pass -WindRiverRoot."
        }
    $wrenv
    }

# Quotes one argument for the 'call' line of a cmd.exe batch file: wrapped in
# double quotes when it holds anything cmd would act on, and % quadrupled, because
# a batch line expands % even inside quotes and 'call' expands it a second time.
function ConvertTo-BatchArgument
    {
    param([Parameter(Mandatory)] [AllowEmptyString()] [string] $Argument)

    if ($Argument.Contains('"'))
        {
        throw "An argument contains a double quote, which cannot be passed safely: $Argument"
        }
    $escaped = $Argument.Replace('%', '%%%%')
    if ($escaped -eq '' -or $escaped -match '[\s&|<>^(),;=]')
        {
        $escaped = '"' + $escaped + '"'
        }
    $escaped
    }

# Runs one command inside the Wind River environment for the given profile,
# returning its stdout and stderr as plain lines, and fails on a non-zero exit
# code. The command itself is echoed to the host, so a caller capturing the
# output gets only the tool's.
#
# The command goes into a temporary batch file rather than onto wrenv's command
# line: wrenv expands $(NAME) in its own arguments, which would consume a build
# macro reference such as $(VX_OS_LIBS) before vxprj saw it.
#
# Under Windows PowerShell 5.1 a redirected stderr line is an error record, which
# 'Stop' would turn into a failure on the first compiler warning, so only the
# exit code decides.
function Invoke-WindRiver
    {
    param(
        [Parameter(Mandatory)] [string] $WindRiverRoot,
        [Parameter(Mandatory)] [string] $WindRiverProfile,
        [Parameter(Mandatory)] [string[]] $Command
    )

    $wrenv = Get-WrenvPath -WindRiverRoot $WindRiverRoot
    $commandPrompt = Join-Path $env:SystemRoot 'System32\cmd.exe'
    Write-Host "> $($Command -join ' ')"

    $batchFile = Join-Path ([System.IO.Path]::GetTempPath()) ("solidsyslog-wr-{0}.bat" -f [guid]::NewGuid())
    $line = ($Command | ForEach-Object { ConvertTo-BatchArgument $_ }) -join ' '
    # call: vxprj is itself a batch file, and without call control would not return.
    [System.IO.File]::WriteAllText($batchFile,
        "@echo off`r`ncall $line`r`nexit /b %ERRORLEVEL%`r`n", [System.Text.Encoding]::ASCII)
    try
        {
        $ErrorActionPreference = 'Continue'
        & $wrenv -p $WindRiverProfile $commandPrompt /d /c $batchFile 2>&1 |
            ForEach-Object { "$_" }
        $exitCode = $LASTEXITCODE
        $ErrorActionPreference = 'Stop'
        }
    finally
        {
        Remove-Item -LiteralPath $batchFile -Force -ErrorAction SilentlyContinue
        }
    if ($exitCode -ne 0)
        {
        throw "Failed with exit code ${exitCode}: $($Command -join ' ')"
        }
    }
