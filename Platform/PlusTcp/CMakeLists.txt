# FreeRTOS-Plus-TCP adapter pack — shipped as sources, not a precompiled library.
#
# FreeRTOS and Mbed TLS are header-configured: the integrator's
# FreeRTOSConfig.h / FreeRTOSIPConfig.h flows through upstream headers and
# changes the size/layout of TCBs, sockets, and IP-task internals. Compiling
# our adapters once into libSolidSyslog.a would silently break consumers
# that use a different config.
#
# Instead, this is an INTERFACE library — each consumer (Bdd/Targets/FreeRtos,
# Tests/FreeRtos/*Test, downstream integrator apps) recompiles the adapter
# sources with its own config on the include path. Mirrors Platform/FreeRtos/
# (kernel primitives — Mutex, SysUpTime) and Platform/MbedTls/ — see
# project_header_configured_platforms memory.

add_library(SolidSyslogPlusTcp INTERFACE)

target_include_directories(SolidSyslogPlusTcp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Interface/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
