# VxWorks64Fakes - a host-side stand-in for the subset of the VxWorks 6.4 kernel
# API that Platform/VxWorks64 calls, used by Tests/VxWorks64/*Test executables.
# The headers carry the names and prototypes of the public API reference and
# nothing else; no Wind River header or library is held in this repository.
#
# The stand-in can drift from the real API; target builds catch that.
#
# Sources are listed by the test executables that need them rather than built
# into a library here, matching CmsisRtosFakes; CMake only requires this file to
# exist.
