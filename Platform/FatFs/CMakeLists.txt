# FatFs adapter pack — shipped as sources, not a precompiled library.
#
# FatFs is header-configured: the integrator's ffconf.h flows through
# upstream headers and changes the size/layout of FATFS and FIL objects
# and the visible API surface. Compiling our adapter once into
# libSolidSyslog.a would silently break consumers that use a different
# ffconf.h.
#
# Instead, this is an INTERFACE library — each consumer (Bdd/Targets/FreeRtos,
# Tests/FatFs/*Test, downstream integrator apps) recompiles the adapter
# sources with its own ffconf.h on the include path. Mirrors the
# Platform/FreeRtos/ pack — see project_header_configured_platforms memory.

add_library(SolidSyslogFatFs INTERFACE)

# No adapter sources at S08.05 slice 1 — first content
# (SolidSyslogFatFsFile.c) lands at slice 2. The INTERFACE target exists
# so the directory and CMake wiring are in place from the plumbing slice.

target_include_directories(SolidSyslogFatFs INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Interface/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
