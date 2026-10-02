<#
.SYNOPSIS
Boots the SolidSyslog VxWorks 6.4 BDD target's ROM image in QEMU (Malta).

.DESCRIPTION
Starts one Malta 4Kc guest with a PCnet adapter on QEMU's user-mode network,
which lets the guest reach the host and beyond through NAT.

With -WaitFor, the console goes to a log file; the script waits for that text,
stops QEMU, and fails if it does not appear in time. That is the boot check.

With -SerialTcp host:port, the console connects out to that address instead and
the script leaves QEMU running and returns its process.

.EXAMPLE
.\Start-VxWorks64Qemu.ps1 -WaitFor 'SolidSyslog VxWorks 6.4 BDD target: Core ran'
.\Start-VxWorks64Qemu.ps1 -SerialTcp 192.168.1.20:5600
#>
[CmdletBinding(DefaultParameterSetName = 'BootCheck')]
param(
    [string] $QemuPath = 'C:\Program Files\qemu\qemu-system-mips.exe',
    [ValidateSet('sfdiab', 'sfgnu')]
    [string] $Tool = 'sfdiab',
    [string] $RomPath,
    [Parameter(Mandatory, ParameterSetName = 'BootCheck')]
    [string] $WaitFor,
    [Parameter(ParameterSetName = 'BootCheck')]
    [ValidateRange(1, 600)]
    [int] $TimeoutSeconds = 60,
    [Parameter(Mandatory, ParameterSetName = 'Serial')]
    [ValidatePattern('^[^:\s]+:\d+$')]
    [string] $SerialTcp
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'VxWorks64Tools.ps1')

if (-not $RomPath)
    {
    $RomPath = Join-Path (Get-DefaultProjectDirectory -Tool $Tool) 'default_rom\vxWorks_rom.bin'
    }
foreach ($file in @($QemuPath, $RomPath))
    {
    if (-not (Test-Path -LiteralPath $file -PathType Leaf))
        {
        throw "Not found: $file"
        }
    }

function ConvertTo-QemuPath([string] $Path)
    {
    [System.IO.Path]::GetFullPath($Path).Replace('\', '/')
    }

# Windows PowerShell 5.1 joins -ArgumentList with spaces and quotes nothing.
function Join-Arguments([string[]] $Arguments)
    {
    ($Arguments | ForEach-Object { if ($_ -match '\s') { "`"$_`"" } else { $_ } }) -join ' '
    }

$arguments = @(
    '-M', 'malta', '-cpu', '4Kc', '-m', '32M',
    '-bios', (ConvertTo-QemuPath $RomPath),
    '-nodefaults', '-display', 'none', '-vga', 'none', '-monitor', 'none',
    '-no-reboot',
    '-netdev', 'user,id=net0,ipv6=off',
    # The BSP routes the onboard Ethernet interrupt for PCI device 11.
    '-device', 'pcnet,netdev=net0,addr=0xb,rombar=0'
)

if ($PSCmdlet.ParameterSetName -eq 'Serial')
    {
    $arguments += @('-serial', "tcp:$SerialTcp", '-serial', 'null')
    $qemu = Start-Process -FilePath $QemuPath -ArgumentList (Join-Arguments $arguments) `
        -WindowStyle Hidden -PassThru
    Write-Host "QEMU running (PID $($qemu.Id)), console connecting to $SerialTcp"
    return $qemu
    }

$runDirectory = Join-Path (Split-Path -Parent $RomPath) ('qemu-' +
    (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssZ'))
$null = New-Item -ItemType Directory -Path $runDirectory
$consoleLog = Join-Path $runDirectory 'console.log'
$arguments += @(
    '-chardev', "file,id=console0,path=$(ConvertTo-QemuPath $consoleLog)",
    '-serial', 'chardev:console0', '-serial', 'null'
)

function Read-Console
    {
    if (-not (Test-Path -LiteralPath $consoleLog -PathType Leaf)) { return '' }
    try
        {
        $stream = [System.IO.File]::Open($consoleLog, 'Open', 'Read', 'ReadWrite')
        try { return (New-Object System.IO.StreamReader($stream)).ReadToEnd() }
        finally { $stream.Dispose() }
        }
    catch [System.IO.IOException] { return '' }
    }

$qemu = Start-Process -FilePath $QemuPath -ArgumentList (Join-Arguments $arguments) `
    -RedirectStandardError (Join-Path $runDirectory 'qemu-stderr.log') `
    -WindowStyle Hidden -PassThru
$found = $false
try
    {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while (-not $found -and [DateTime]::UtcNow -lt $deadline)
        {
        if ($qemu.HasExited)
            {
            throw "QEMU exited with code $($qemu.ExitCode) before the marker appeared."
            }
        $found = (Read-Console).Contains($WaitFor)
        if (-not $found) { Start-Sleep -Milliseconds 250 }
        }
    }
finally
    {
    if (-not $qemu.HasExited)
        {
        Stop-Process -Id $qemu.Id -Force
        $qemu.WaitForExit()
        }
    }

Write-Host "Console log: $consoleLog"
if (-not $found)
    {
    throw "'$WaitFor' did not appear within $TimeoutSeconds s."
    }
Write-Host "PASS: '$WaitFor'"
