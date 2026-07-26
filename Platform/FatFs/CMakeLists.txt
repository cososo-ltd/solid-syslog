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
#
# The pack CARRIES its sources via target_sources(INTERFACE) (S30.02): a
# consumer that links SolidSyslog::FatFs compiles the adapter into its own
# target, seeing its own ffconf.h — no per-file listing. The consumer still
# points at its own FatFs tree and supplies its own diskio.c.

add_library(SolidSyslogFatFs INTERFACE)

target_sources(SolidSyslogFatFs INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogFatFsFile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogFatFsFileStatic.c
)

target_include_directories(SolidSyslogFatFs INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogFatFsFilePrivate.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h
)

target_link_libraries(SolidSyslogFatFs INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::FatFs ALIAS SolidSyslogFatFs)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Interface/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
