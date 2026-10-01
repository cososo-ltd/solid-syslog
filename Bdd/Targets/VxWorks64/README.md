# VxWorks 6.4 BDD target

The SolidSyslog BDD target for VxWorks 6.4: a kernel VxWorks Image Project (VIP)
that boots in QEMU's Malta machine. The scripts here create the VIP, build it
with SolidSyslog linked in, and boot it.

Nothing the Wind River tools generate is kept in the repository. The VIP is
created from scratch by script, under `build\`, which git ignores.

## Prerequisites

- A Windows host.
- A licensed Wind River VxWorks 6.4 installation with the Diab toolchain, by
  default at `C:\WindRiver`. The GNU toolchain is optional.
- The BSP `malta4kc_qemu_mips32sf`, installed beside the others in
  `<WindRiverRoot>\vxworks-6.4\target\config\`. It is a bug-fixed version of
  Wind River's `malta4kc_mips32sf` BSP.
- QEMU, installed separately, with `qemu-system-mips.exe` - by default at
  `C:\Program Files\qemu\`.
- This checkout at a path without whitespace, which the Wind River make rules
  cannot handle.

## Create, build and boot

From PowerShell in this directory:

```powershell
.\New-VxWorks64Vip.ps1
.\Build-VxWorks64Vip.ps1
.\Start-VxWorks64Qemu.ps1 -WaitFor 'SolidSyslog VxWorks 6.4 BDD target: Core ran'
```

`New-VxWorks64Vip.ps1` creates the VIP with `vxprj` and adds the BDD target's
component (`99SolidSyslogVxWorks64Bdd.cdf`) and source
(`BddTargetVxWorks64.c`). Pass `-Force` to replace an existing project, and
`-Tool sfgnu` for the GNU toolchain.

`Build-VxWorks64Vip.ps1` builds the SolidSyslog library, then the `default_rom`
image and the raw `vxWorks_rom.bin` that QEMU loads, and prints the image's
SHA-256. The full build output is kept beside the project as
`build-default_rom.log`, and the last lines on the console count every compiler
diagnostic in it, or report none.

`Start-VxWorks64Qemu.ps1 -WaitFor` boots the image, waits for the text on the
console, then stops QEMU. It fails if the text does not appear within
`-TimeoutSeconds` (default 60). With `-SerialTcp host:port` instead, the console
connects out to that address and QEMU is left running.

Each script describes its parameters: `Get-Help .\<script>.ps1 -Detailed`.

## How SolidSyslog gets into the image

The build script runs `Platform/VxWorks64/solidsyslog.makefile` against the
project's own Makefile, so the library is compiled with the build
specification's compiler and flags into `solidsyslog\` in the project. The
creation script sets two of the project's build macros with
`vxprj buildmacro set`: `CFLAGS` gains the SolidSyslog include directory and
`Platform/VxWorks64/Compat`, and `LIBS` names the library ahead of
`$(VX_OS_LIBS)`.

The kernel header tree has no `<stdint.h>` or `<stdbool.h>`, which the
SolidSyslog headers include. `Platform/VxWorks64/Compat` supplies both, for the
library and for the project's own sources alike.
