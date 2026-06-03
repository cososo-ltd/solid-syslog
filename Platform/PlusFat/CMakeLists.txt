# FreeRTOS-Plus-FAT adapter pack — shipped as sources, not a precompiled library.
#
# Plus-FAT is header-configured: the integrator's FreeRTOSFATConfig.h flows
# through upstream headers and changes the size/layout of the FF_FILE /
# FF_IOManager objects and the visible API surface. Plus-FAT is also FreeRTOS-
# coupled (ff_headers.h pulls FreeRTOS.h / task.h / semphr.h), so compiling our
# adapter once into libSolidSyslog.a would silently break consumers that use a
# different FreeRTOSFATConfig.h or kernel config.
#
# Instead, this is an INTERFACE library — each consumer (Bdd/Targets/FreeRtos,
# Tests/PlusFat/*Test, downstream integrator apps) recompiles the adapter
# sources with its own FreeRTOSFATConfig.h on the include path. Mirrors the
# Platform/FatFs/ and Platform/FreeRtos/ packs — see
# project_header_configured_platforms memory.

add_library(SolidSyslogPlusFat INTERFACE)

target_include_directories(SolidSyslogPlusFat INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Interface/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
