# CmsisRtosFakes — a host-side stand-in for the whole CMSIS-RTOS2 API used by
# Tests/CmsisRtos/*Test executables, cmsis_os2.h included. The real header is
# pure declarations, so unlike the other upstream fakes this one needs no tree
# behind it and the suite builds wherever the compiler does.
#
# The price is that the stand-in carries only the subset the adapters call, so
# it can drift from the real API. The cross-compiled BDD target is what closes
# that: there the same sources compile against the header the RTOS ships.
#
# Sources are listed by the test executables that need them rather than built
# into a library here, matching FreeRtosFakes; CMake only requires this file to
# exist.
