# FreeRTOS adapter pack — shipped as sources, not a precompiled library.
#
# FreeRTOS and Mbed TLS are header-configured: the integrator's
# FreeRTOSConfig.h / mbedtls_config.h flows through upstream headers and
# changes the size/layout of TCBs, semaphores, and SSL contexts. Compiling
# our adapters once into libSolidSyslog.a would silently break consumers
# that use a different config.
#
# Instead, this is an INTERFACE library — each consumer (Example/FreeRtos/*,
# Tests/FreeRtos/*Test, downstream integrator apps) recompiles the adapter
# sources with its own config on the include path.
#
# Posix / Windows / OpenSSL keep the existing PRIVATE-into-${PROJECT_NAME}
# pattern; their ABIs are OS-level, not header-configured. Don't retrofit
# INTERFACE across the tree.

add_library(SolidSyslogFreeRtos INTERFACE)

# No adapter sources at S08.01 — first content (SolidSyslogFreeRtosMutex.c)
# lands at S08.04. The INTERFACE target exists so the directory and CMake
# wiring are in place from day one.

target_include_directories(SolidSyslogFreeRtos INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Interface/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
