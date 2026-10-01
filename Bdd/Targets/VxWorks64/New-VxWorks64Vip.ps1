<#
.SYNOPSIS
Creates the SolidSyslog VxWorks 6.4 BDD target's VxWorks Image Project.

.DESCRIPTION
Creates a fresh VIP for the BSP and toolchain with vxprj, then adds the BDD
target: its component description, its source, and solidsyslog.makefile, which
builds SolidSyslog and links it in. Nothing it generates is kept in the
repository; the default location is under build\, which git ignores.

.EXAMPLE
.\New-VxWorks64Vip.ps1
.\New-VxWorks64Vip.ps1 -Tool gnu -Force
#>
[CmdletBinding()]
param(
    [string] $WindRiverRoot = 'C:\WindRiver',
    [string] $WindRiverProfile = 'vxworks-6.4',
    [string] $Bsp = 'malta4kc_qemu_mips32sf',
    [ValidateSet('sfdiab', 'sfgnu')]
    [string] $Tool = 'sfdiab',
    [string] $ProjectDirectory,
    # Delete an existing project in ProjectDirectory and create it again.
    [switch] $Force
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'VxWorks64Tools.ps1')

if (-not $ProjectDirectory)
    {
    $ProjectDirectory = Get-DefaultProjectDirectory -Tool $Tool
    }
$ProjectDirectory = [System.IO.Path]::GetFullPath($ProjectDirectory)
$projectFile = Get-ProjectFile -ProjectDirectory $ProjectDirectory
Assert-NoWhitespace -Path $ProjectDirectory -Description 'The project directory'

$bspDirectory = Join-Path $WindRiverRoot "vxworks-6.4\target\config\$Bsp"
if (-not (Test-Path -LiteralPath $bspDirectory -PathType Container))
    {
    throw "BSP $Bsp is not installed: $bspDirectory not found."
    }

if (Test-Path -LiteralPath $ProjectDirectory)
    {
    if (-not $Force)
        {
        throw "$ProjectDirectory already exists. Pass -Force to create it again."
        }
    Remove-Item -LiteralPath $ProjectDirectory -Recurse -Force
    }

$vxprj = @{ WindRiverRoot = $WindRiverRoot; WindRiverProfile = $WindRiverProfile }

Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'create', $Bsp, $Tool, $projectFile)

# The generated Makefile includes every *.makefile in the project directory, and
# vxprj reads component descriptions from it.
foreach ($file in @(
        (Join-Path $PSScriptRoot '99SolidSyslogVxWorks64Bdd.cdf'),
        (Join-Path $PSScriptRoot 'BddTargetVxWorks64.c'),
        (Join-Path $script:RepositoryRoot 'Platform\VxWorks64\solidsyslog.makefile')
    ))
    {
    Copy-Item -LiteralPath $file -Destination $ProjectDirectory
    }

Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'file', 'add', $projectFile,
    (Join-Path $ProjectDirectory 'BddTargetVxWorks64.c'))
Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'component', 'add', $projectFile,
    'INCLUDE_SOLIDSYSLOG_VXWORKS64_BDD')

Write-Host "Created $projectFile"
