<#
.SYNOPSIS
Builds the SolidSyslog VxWorks 6.4 BDD target's VxWorks Image Project.

.DESCRIPTION
Builds a VIP made by New-VxWorks64Vip.ps1. The SolidSyslog library is built
first, from this checkout, by running solidsyslog.makefile against the project's
own Makefile - so it compiles with the build specification's compiler and flags.
For the default_rom
build specification it also produces vxWorks_rom.bin, the raw image QEMU's
-bios option loads. The full build output is kept beside the project as
build-<spec>.log.

.EXAMPLE
.\Build-VxWorks64Vip.ps1
.\Build-VxWorks64Vip.ps1 -Tool sfgnu -Clean
#>
[CmdletBinding()]
param(
    [string] $WindRiverRoot = 'C:\WindRiver',
    [string] $WindRiverProfile = 'vxworks-6.4',
    [ValidateSet('sfdiab', 'sfgnu')]
    [string] $Tool = 'sfdiab',
    [string] $ProjectDirectory,
    [ValidateSet('default', 'default_rom')]
    [string] $BuildSpec = 'default_rom',
    # Platforms to build beside Core, as SOLIDSYSLOG_PLATFORMS.
    [string] $Platforms = '',
    [switch] $Clean
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'VxWorks64Tools.ps1')

if (-not $ProjectDirectory)
    {
    $ProjectDirectory = Get-DefaultProjectDirectory -Tool $Tool
    }
$ProjectDirectory = [System.IO.Path]::GetFullPath($ProjectDirectory)
$projectFile = Get-ProjectFile -ProjectDirectory $ProjectDirectory
if (-not (Test-Path -LiteralPath $projectFile -PathType Leaf))
    {
    throw "No project at $projectFile - run New-VxWorks64Vip.ps1 first."
    }
Assert-NoWhitespace -Path $script:RepositoryRoot -Description 'The SolidSyslog checkout path'

$logPath = Join-Path $ProjectDirectory "build-$BuildSpec.log"
$vxprj = @{ WindRiverRoot = $WindRiverRoot; WindRiverProfile = $WindRiverProfile }

$previousDir = $env:SOLIDSYSLOG_DIR
$previousPlatforms = $env:SOLIDSYSLOG_PLATFORMS
try
    {
    # Read by solidsyslog.makefile inside the project's build.
    $env:SOLIDSYSLOG_DIR = $script:RepositoryRoot.Replace('\', '/')
    $env:SOLIDSYSLOG_PLATFORMS = $Platforms

    $libraryDirectory = Get-LibraryDirectory -ProjectDirectory $ProjectDirectory
    $library = Join-Path $libraryDirectory 'libsolidsyslog.a'
    $specDirectory = Join-Path $ProjectDirectory $BuildSpec
    if ($Clean -and (Test-Path -LiteralPath $libraryDirectory))
        {
        Remove-Item -LiteralPath $libraryDirectory -Recurse -Force
        }

    & {
        Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'build', 'set', $projectFile, $BuildSpec)
        if ($Clean)
            {
            Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'build', $projectFile, 'clean')
            }

        # The project's Makefile supplies CC, AR, TOOL_FAMILY and CFLAGS for the
        # build specification; solidsyslog.makefile builds the library with them.
        Invoke-WindRiver @vxprj -Command @('make',
            '-C', (ConvertTo-MakePath $ProjectDirectory),
            '-f', 'Makefile',
            '-f', (ConvertTo-MakePath (Join-Path $script:RepositoryRoot 'Platform\VxWorks64\solidsyslog.makefile')),
            "BUILD_SPEC=$BuildSpec",
            "SOLIDSYSLOG_BUILD_DIR=$(ConvertTo-MakePath $libraryDirectory)",
            'solidsyslog_library')

        # The image does not depend on the library in the project's rules, so a
        # rebuilt library would not relink it. Remove the stale images instead.
        $images = @(Get-ChildItem -LiteralPath $specDirectory -Filter 'vxWorks*' -File -ErrorAction SilentlyContinue)
        $oldestImage = $images | Sort-Object LastWriteTimeUtc | Select-Object -First 1
        if ($oldestImage -and
            (Get-Item -LiteralPath $library).LastWriteTimeUtc -gt $oldestImage.LastWriteTimeUtc)
            {
            Write-Host 'The SolidSyslog library changed - removing the old images to force a relink.'
            $images | Remove-Item -Force
            }

        Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'build', $projectFile)
        if ($BuildSpec -eq 'default_rom')
            {
            Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'build', $projectFile, 'vxWorks_rom.bin')
            }
        } *>&1 | Tee-Object -FilePath $logPath
    }
finally
    {
    $env:SOLIDSYSLOG_DIR = $previousDir
    $env:SOLIDSYSLOG_PLATFORMS = $previousPlatforms
    }

$image = if ($BuildSpec -eq 'default_rom') { 'vxWorks_rom.bin' } else { 'vxWorks' }
$imagePath = Join-Path $ProjectDirectory "$BuildSpec\$image"
if (-not (Test-Path -LiteralPath $imagePath -PathType Leaf))
    {
    throw "The build finished without producing $imagePath"
    }

$hash = (Get-FileHash -LiteralPath $imagePath -Algorithm SHA256).Hash
Write-Host "Built $imagePath"
Write-Host "SHA-256 $hash"
Write-Host "Log $logPath"
