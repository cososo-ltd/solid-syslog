<#
.SYNOPSIS
Creates the SolidSyslog VxWorks 6.4 BDD target's VxWorks Image Project.

.DESCRIPTION
Creates a fresh VIP for the BSP and toolchain with vxprj, adds the BDD target's
component description and source, and sets each build specification's CFLAGS
and LIBS so the project compiles against the SolidSyslog headers and links the
library Build-VxWorks64Vip.ps1 builds. Nothing it generates is kept in the
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

# vxprj reads component descriptions from the project directory.
foreach ($file in @(
        (Join-Path $PSScriptRoot '99SolidSyslogVxWorks64Bdd.cdf'),
        (Join-Path $PSScriptRoot 'BddTargetVxWorks64Headers.c')
    ))
    {
    Copy-Item -LiteralPath $file -Destination $ProjectDirectory
    }

# The project's only source is the C89 header check. The BDD target is C99 and
# is linked from its own archive, which Build-VxWorks64Vip.ps1 builds.
Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'file', 'add', $projectFile,
    (Join-Path $ProjectDirectory 'BddTargetVxWorks64Headers.c'))
Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'component', 'add', $projectFile,
    'INCLUDE_SOLIDSYSLOG_VXWORKS64_BDD')

# The network the BDD target sends over: the IPv4 stack with UDP, TCP and
# sockets, select for the bounded TCP connect, the host table the resolver
# consults, routing for the default route the target adds, the END driver for
# QEMU's PCnet adapter (lnPci), and the boot-line components that attach the
# device the boot line names. One at a time, so a component the default project
# already has is simply kept.
foreach ($component in @(
        'INCLUDE_NETWORK', 'INCLUDE_IPV4', 'INCLUDE_UDPV4', 'INCLUDE_TCPV4', 'INCLUDE_BSD_SOCKET',
        'INCLUDE_SOCKLIB', 'INCLUDE_SELECT',
        'INCLUDE_END', 'INCLUDE_MUX', 'INCLUDE_IPATTACH', 'INCLUDE_PCI', 'INCLUDE_LN_97X_END',
        'INCLUDE_IFLIB', 'INCLUDE_INETLIB', 'INCLUDE_HOST_TBL', 'INCLUDE_ROUTE', 'INCLUDE_OLDROUTE',
        'INCLUDE_BOOT_LINE_INIT', 'INCLUDE_NET_INIT', 'INCLUDE_NET_BOOT', 'INCLUDE_NET_BOOT_CONFIG',
        'INCLUDE_NET_HOST_SETUP', 'INCLUDE_ADDIF'
    ))
    {
    Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'component', 'add', $projectFile, $component)
    }

# The disk the file store lives on: the ATA driver for the board's PIIX4 IDE
# controller, the extended block device layer it creates its disk through, the
# file system monitor that names the volume /ata0a and puts rawFs on a disk it
# cannot recognise, and dosFs with the formatter the target runs on a blank disk.
# The dosFs cache stays in, so the file's own sync is what reaches the disk.
foreach ($component in @(
        'INCLUDE_IPIIX4PCI', 'INCLUDE_ATA',
        'INCLUDE_XBD', 'INCLUDE_XBD_BLK_DEV', 'INCLUDE_FS_MONITOR', 'INCLUDE_RAWFS',
        'INCLUDE_DOSFS_MAIN', 'INCLUDE_DOSFS_FAT', 'INCLUDE_DOSFS_DIR_VFAT', 'INCLUDE_DOSFS_FMT',
        'INCLUDE_DOSFS_CACHE'
    ))
    {
    Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'component', 'add', $projectFile, $component)
    }

# QEMU loads the ROM image directly, so nothing boots over the network and the
# boot line's addresses are never used. INCLUDE_ADDIF puts the PCnet interface
# on QEMU's user network instead, where the guest is always 10.0.2.15/24.
Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'parameter', 'setstring', $projectFile, 'ADDIF_NAME', 'lnPci')
Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'parameter', 'set', $projectFile, 'ADDIF_NUM', '0')
Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'parameter', 'setstring', $projectFile, 'ADDIF_ADDR', '10.0.2.15')
Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'parameter', 'set', $projectFile, 'ADDIF_MASK', '0xffffff00')

# Give every build specification the SolidSyslog headers - Core's and the
# VxWorks64 pack's - and the library. The headers need the C99 <stdint.h> and
# <stdbool.h> the kernel tree lacks, from the pack's Compat directory, placed
# last so it only fills gaps. The BDD target's archive goes first, then the
# library it calls, then the OS libraries both call into. buildmacro acts on the
# current build specification, so each is selected in turn.
$includes = '-I' + (ConvertTo-MakePath (Join-Path $script:RepositoryRoot 'Core\Interface')) +
    ' -I' + (ConvertTo-MakePath (Join-Path $script:RepositoryRoot 'Platform\VxWorks64\Interface')) +
    ' -I' + (ConvertTo-MakePath (Join-Path $script:RepositoryRoot 'Platform\VxWorks64\Compat'))
$library = ConvertTo-MakePath (Join-Path (Get-LibraryDirectory -ProjectDirectory $ProjectDirectory) 'libsolidsyslog.a')
$bddTarget = ConvertTo-MakePath (Join-Path (Get-BddTargetDirectory -ProjectDirectory $ProjectDirectory) 'libsolidsyslogbdd.a')
foreach ($buildSpec in @('default', 'default_rom'))
    {
    Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'build', 'set', $projectFile, $buildSpec)

    $cflags = ((Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'buildmacro', 'get',
        $projectFile, 'CFLAGS')) -join ' ').Trim()
    if (-not $cflags)
        {
        throw "vxprj returned no CFLAGS for build specification $buildSpec."
        }

    Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'buildmacro', 'set', $projectFile,
        'CFLAGS', "$cflags $includes")
    Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'buildmacro', 'set', $projectFile,
        'LIBS', "$bddTarget $library `$(VX_OS_LIBS)")

    # With Diab, Wind River's pciIntLib.c, compiled into the BSP's sysLib.c,
    # raises dcc:1606 (a condition always true or false), and the BSP's
    # sysBusPci.c raises dcc:1741 by defining USB again after the ATA driver's
    # header has defined it. PROJECT_BSP_FLAGS_EXTRA reaches Wind River's sources
    # and the ones vxprj generates - the BSP, romStart.c, prjConfig.c and
    # linkSyms.c - so the project's own sources keep both warnings. -ei is a Diab
    # option, so a GNU project is left as it is.
    if ($Tool -eq 'sfdiab')
        {
        $bspFlags = ((Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'buildmacro', 'get',
            $projectFile, 'PROJECT_BSP_FLAGS_EXTRA')) -join ' ').Trim()
        Invoke-WindRiver @vxprj -Command @('vxprj.bat', 'buildmacro', 'set', $projectFile,
            'PROJECT_BSP_FLAGS_EXTRA', "$bspFlags -ei1606,1741".Trim())
        }
    }

Write-Host "Created $projectFile"
