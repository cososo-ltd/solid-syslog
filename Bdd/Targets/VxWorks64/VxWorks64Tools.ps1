# Helpers shared by the VxWorks 6.4 recipe scripts. Dot-source, do not run.
# Written for Windows PowerShell 5.1.

Set-StrictMode -Version Latest

$script:RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path

function Get-DefaultProjectDirectory
    {
    param([Parameter(Mandatory)] [string] $Tool)

    Join-Path $script:RepositoryRoot "build\vxworks64\$Tool"
    }

function Get-ProjectFile
    {
    param([Parameter(Mandatory)] [string] $ProjectDirectory)

    Join-Path $ProjectDirectory 'SolidSyslogVxWorks64.wpj'
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

# Runs one command inside the Wind River environment for the given profile,
# writing its stdout and stderr as plain lines, and fails on a non-zero exit
# code. Under Windows PowerShell 5.1 a redirected stderr line is an error
# record, which 'Stop' would turn into a failure on the first compiler warning,
# so only the exit code decides.
function Invoke-WindRiver
    {
    param(
        [Parameter(Mandatory)] [string] $WindRiverRoot,
        [Parameter(Mandatory)] [string] $WindRiverProfile,
        [Parameter(Mandatory)] [string[]] $Command
    )

    $wrenv = Get-WrenvPath -WindRiverRoot $WindRiverRoot
    $commandPrompt = Join-Path $env:SystemRoot 'System32\cmd.exe'
    Write-Output "> $($Command -join ' ')"
    $ErrorActionPreference = 'Continue'
    & $wrenv -p $WindRiverProfile $commandPrompt /d /c @Command 2>&1 |
        ForEach-Object { "$_" }
    $ErrorActionPreference = 'Stop'
    if ($LASTEXITCODE -ne 0)
        {
        throw "Failed with exit code ${LASTEXITCODE}: $($Command -join ' ')"
        }
    }
