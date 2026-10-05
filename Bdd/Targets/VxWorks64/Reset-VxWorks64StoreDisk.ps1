<#
.SYNOPSIS
Removes the VxWorks 6.4 BDD target's store disk image, so the next QEMU start
gives the target a blank disk.

.DESCRIPTION
The file store outlives a QEMU restart on purpose, so one scenario's records
would otherwise be there for the next. The runner's store-reset job runs this
before each store scenario. QEMU must not be running: it holds the image open.

.EXAMPLE
.\Reset-VxWorks64StoreDisk.ps1
#>
[CmdletBinding()]
param(
    [string] $DiskPath
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'VxWorks64Tools.ps1')

if (-not $DiskPath)
    {
    $DiskPath = Get-StoreDiskPath
    }
if (Test-Path -LiteralPath $DiskPath -PathType Leaf)
    {
    Remove-Item -LiteralPath $DiskPath
    Write-Host "Removed $DiskPath"
    }
else
    {
    Write-Host 'No store disk to remove'
    }
