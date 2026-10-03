<#
.SYNOPSIS
Runs jobs for the VxWorks 6.4 BDD target, collected from the job service on the
development machine.

.DESCRIPTION
Run by hand on the build machine, in its read-only clone; stop it with Ctrl+C.
It polls the job service over HTTPS, connecting outwards only, trusts only the
certificate whose thumbprint it is given, and runs one job at a time from a
fixed set:

  checkout ref=<branch or commit>   fetch, then force the clone to that commit
  build                             New-VxWorks64Vip.ps1 -Force, then
                                    Build-VxWorks64Vip.ps1 -Clean
  boot-check marker=<text>          Start-VxWorks64Qemu.ps1 -WaitFor <text>
  qemu-start                        QEMU, its console connecting out to the
                                    service's host on -SerialPort
  qemu-stop                         stops the QEMU qemu-start began
  status                            the clone's commit, and whether QEMU runs

Anything else is refused. The scripts a job runs are the clone's own, in a
separate PowerShell process, and their output streams back as the job's log.
A checkout cannot change the runner while it runs: PowerShell parses a script,
and everything it dot-sources, when it loads them.

.EXAMPLE
.\Start-VxWorks64Runner.ps1 -Service https://192.168.1.20:8765 -Thumbprint 0123...CDEF
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidatePattern('^https://[^/\s]+$')]
    [string] $Service,
    [Parameter(Mandatory)]
    [ValidatePattern('^[0-9A-Fa-f]{40}$')]
    [string] $Thumbprint,
    [string] $TokenFile = (Join-Path $env:USERPROFILE '.solidsyslog-runner\token'),
    [ValidateRange(1, 65535)]
    [int] $SerialPort = 8766,
    [ValidateRange(1, 60)]
    [int] $PollSeconds = 3
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'VxWorks64Runner.ps1')

$clone = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..\..'))
$recipe = Join-Path $clone 'Bdd\Targets\VxWorks64'
$token = (Get-Content -LiteralPath $TokenFile -Raw).Trim()
$serviceHost = ([Uri] $Service).Host
$script:currentJob = $null
$script:qemuProcessId = $null

[SolidSyslogRunnerPinning]::Install($Thumbprint)

function Invoke-Service
    {
    param(
        [Parameter(Mandatory)] [string] $Method,
        [Parameter(Mandatory)] [string] $Path,
        [byte[]] $Body,
        [string] $ContentType = 'application/json'
    )

    $request = @{
        Method = $Method; Uri = "$Service$Path"; UseBasicParsing = $true
        Headers = @{ 'X-Runner-Token' = $token }
    }
    if ($Body)
        {
        $request.Body = $Body
        $request.ContentType = $ContentType
        }
    Invoke-WebRequest @request
    }

function Write-JobLog
    {
    param([Parameter(Mandatory)] [AllowEmptyString()] [string] $Text)

    if ($Text)
        {
        Write-Host -NoNewline $Text
        Invoke-Service -Method POST -Path "/jobs/$($script:currentJob.id)/log" `
            -Body ([System.Text.Encoding]::UTF8.GetBytes($Text)) -ContentType 'text/plain; charset=utf-8' | Out-Null
        }
    }

# Windows PowerShell 5.1 joins -ArgumentList with spaces and quotes nothing. The
# values reaching here carry no quotes: Test-RunnerArgumentValue saw to that.
function Join-Arguments([string[]] $Arguments)
    {
    ($Arguments | ForEach-Object { if ($_ -match '\s') { "`"$_`"" } else { $_ } }) -join ' '
    }

# Runs a program, streaming its output into the job's log as it goes, and
# returns its exit code and everything it wrote.
function Invoke-JobProcess
    {
    param(
        [Parameter(Mandatory)] [string] $FilePath,
        [Parameter(Mandatory)] [string[]] $Arguments
    )

    $outFile = [System.IO.Path]::GetTempFileName()
    $errFile = [System.IO.Path]::GetTempFileName()
    $offsets = @{ $outFile = 0L; $errFile = 0L }
    $all = New-Object System.Text.StringBuilder
    try
        {
        Write-JobLog "> $FilePath $(Join-Arguments $Arguments)`r`n"
        $process = Start-Process -FilePath $FilePath -ArgumentList (Join-Arguments $Arguments) `
            -WorkingDirectory $clone -RedirectStandardOutput $outFile -RedirectStandardError $errFile `
            -WindowStyle Hidden -PassThru
        # Without the handle taken while it runs, Windows PowerShell 5.1 reports
        # no exit code for the process.
        $null = $process.Handle
        $finished = $false
        while (-not $finished)
            {
            $finished = $process.HasExited
            foreach ($file in @($outFile, $errFile))
                {
                $stream = [System.IO.File]::Open($file, 'Open', 'Read', 'ReadWrite')
                try
                    {
                    $null = $stream.Seek($offsets[$file], 'Begin')
                    $text = (New-Object System.IO.StreamReader($stream)).ReadToEnd()
                    $offsets[$file] = $stream.Position
                    }
                finally
                    {
                    $stream.Dispose()
                    }
                $null = $all.Append($text)
                Write-JobLog $text
                }
            if (-not $finished)
                {
                Start-Sleep -Seconds 2
                }
            }
        @{ ExitCode = $process.ExitCode; Output = $all.ToString() }
        }
    finally
        {
        Remove-Item -LiteralPath $outFile, $errFile -Force -ErrorAction SilentlyContinue
        }
    }

function Invoke-RecipeScript
    {
    param(
        [Parameter(Mandatory)] [string] $Name,
        [string[]] $Arguments = @()
    )

    $script = Join-Path $recipe $Name
    if (-not (Test-Path -LiteralPath $script -PathType Leaf))
        {
        throw "This commit has no $Name"
        }
    Invoke-JobProcess -FilePath 'powershell.exe' `
        -Arguments (@('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $script) + $Arguments)
    }

function Invoke-Git([string[]] $Arguments)
    {
    $result = Invoke-JobProcess -FilePath 'git.exe' -Arguments (@('-C', $clone) + $Arguments)
    if ($result.ExitCode -ne 0)
        {
        throw "git $($Arguments -join ' ') failed with exit code $($result.ExitCode)"
        }
    $result.Output.Trim()
    }

function Get-LastLines([string] $Text, [int] $Count)
    {
    (($Text -split "`r?`n") | Where-Object { $_ } | Select-Object -Last $Count) -join "`n"
    }

function Get-QemuProcess
    {
    if ($script:qemuProcessId)
        {
        Get-Process -Id $script:qemuProcessId -ErrorAction SilentlyContinue |
            Where-Object { $_.ProcessName -like 'qemu-system-mips*' }
        }
    }

$actions = @{
    'checkout' = @{ Arguments = @('ref'); Run = {
        param($Arguments)
        $null = Invoke-Git @('fetch', '--prune', 'origin')
        $target = $null
        foreach ($candidate in @("origin/$($Arguments.ref)", $Arguments.ref))
            {
            if (-not $target)
                {
                $result = Invoke-JobProcess -FilePath 'git.exe' `
                    -Arguments @('-C', $clone, 'rev-parse', '--verify', '--quiet', "$candidate^{commit}")
                if ($result.ExitCode -eq 0)
                    {
                    $target = $result.Output.Trim()
                    }
                }
            }
        if (-not $target)
            {
            throw "No branch or commit '$($Arguments.ref)' in origin"
            }
        $null = Invoke-Git @('checkout', '--detach', '--force', $target)
        @{ Outcome = 'succeeded'; Summary = "Clone at $target" } } }

    'build' = @{ Arguments = @(); Run = {
        param($Arguments)
        $created = Invoke-RecipeScript 'New-VxWorks64Vip.ps1' @('-Force')
        if ($created.ExitCode -ne 0)
            {
            @{ Outcome = 'failed'; Summary = (($created.Output -split "`r?`n") | Where-Object { $_ } | Select-Object -First 1) }
            }
        else
            {
            $built = Invoke-RecipeScript 'Build-VxWorks64Vip.ps1' @('-Clean')
            $outcome = if ($built.ExitCode -eq 0) { 'succeeded' } else { 'failed' }
            $diagnostics = $built.Output.LastIndexOf('Diagnostics:')
            $summary = if ($diagnostics -ge 0) { $built.Output.Substring($diagnostics).Trim() } else { Get-LastLines $built.Output 5 }
            @{ Outcome = $outcome; Summary = $summary }
            } } }

    'boot-check' = @{ Arguments = @('marker'); Run = {
        param($Arguments)
        $booted = Invoke-RecipeScript 'Start-VxWorks64Qemu.ps1' @('-WaitFor', $Arguments.marker)
        $outcome = if ($booted.ExitCode -eq 0) { 'succeeded' } else { 'failed' }
        @{ Outcome = $outcome; Summary = Get-LastLines $booted.Output 2 } } }

    'qemu-start' = @{ Arguments = @(); Run = {
        param($Arguments)
        if (Get-QemuProcess)
            {
            throw "QEMU is already running (PID $script:qemuProcessId) - stop it first"
            }
        $started = Invoke-RecipeScript 'Start-VxWorks64Qemu.ps1' @('-SerialTcp', "${serviceHost}:$SerialPort")
        if (($started.ExitCode -ne 0) -or ($started.Output -notmatch 'PID (\d+)'))
            {
            @{ Outcome = 'failed'; Summary = Get-LastLines $started.Output 3 }
            }
        else
            {
            $script:qemuProcessId = [int] $Matches[1]
            @{ Outcome = 'succeeded'; Summary = "QEMU running (PID $script:qemuProcessId), console to ${serviceHost}:$SerialPort" }
            } } }

    'qemu-stop' = @{ Arguments = @(); Run = {
        param($Arguments)
        $qemu = Get-QemuProcess
        if ($qemu)
            {
            Stop-Process -Id $qemu.Id -Force
            $qemu.WaitForExit()
            }
        $script:qemuProcessId = $null
        @{ Outcome = 'succeeded'; Summary = if ($qemu) { "Stopped QEMU (PID $($qemu.Id))" } else { 'QEMU was not running' } } } }

    'status' = @{ Arguments = @(); Run = {
        param($Arguments)
        $commit = Invoke-Git @('rev-parse', 'HEAD')
        $qemu = Get-QemuProcess
        $qemuState = if ($qemu) { "QEMU running (PID $($qemu.Id))" } else { 'QEMU not running' }
        @{ Outcome = 'succeeded'; Summary = "Clone at $commit; $qemuState" } } }
}

# A 404 means the service was restarted and no longer knows the job, so the
# result has nowhere to go; anything else is worth another try.
$sendResult = {
    param($Id, $Body)
    try
        {
        $null = Invoke-Service -Method POST -Path "/jobs/$Id/result" -Body ([System.Text.Encoding]::UTF8.GetBytes($Body))
        }
    catch [System.Net.WebException]
        {
        $response = $_.Exception.Response
        if ($response -and ([int] $response.StatusCode -eq 404))
            {
            Write-Warning "The service no longer knows job $Id; its result is dropped."
            }
        else
            {
            throw
            }
        }
}

# A result the service has not yet accepted is sent again on every pass, before
# any new job is taken: until it arrives, the job stays running there.
$pendingResult = $null
Write-Host "Runner for $clone, polling $Service every $PollSeconds s. Ctrl+C stops it."
while ($true)
    {
    try
        {
        if ($pendingResult)
            {
            $pendingResult = Send-RunnerResult -Pending $pendingResult -Send $sendResult
            if ($pendingResult)
                {
                Start-Sleep -Seconds $PollSeconds
                }
            }
        else
            {
            $response = Invoke-Service -Method GET -Path '/jobs/next'
            if ($response.StatusCode -eq 200)
                {
                $script:currentJob = ConvertFrom-RunnerJobJson $response.Content
                Write-Host "Job $($script:currentJob.id): $($script:currentJob.type)"
                $result = Invoke-RunnerJob -Job $script:currentJob -Actions $actions
                Write-Host "Job $($script:currentJob.id): $($result.Outcome) - $($result.Summary)"
                $body = @{ outcome = $result.Outcome; summary = [string] $result.Summary } | ConvertTo-Json
                $pendingResult = Send-RunnerResult -Pending @{ Id = $script:currentJob.id; Body = $body } -Send $sendResult
                $script:currentJob = $null
                }
            else
                {
                Start-Sleep -Seconds $PollSeconds
                }
            }
        }
    catch
        {
        Write-Warning "Service unreachable or refused: $($_.Exception.Message)"
        Start-Sleep -Seconds $PollSeconds
        }
    }
