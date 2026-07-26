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
#
# The pack CARRIES its sources via target_sources(INTERFACE) (S30.02): a
# consumer that links SolidSyslog::FreeRtos compiles the kernel-primitive
# adapters into its own target, seeing its own FreeRTOSConfig.h — no per-file
# listing. The consumer still points at its own FreeRTOS-Kernel tree.

add_library(SolidSyslogFreeRtos INTERFACE)

# Kernel primitives, independent of the chosen networking backend: the
# static-allocation Mutex and the kernel-tick SysUpTime source.
target_sources(SolidSyslogFreeRtos INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogFreeRtosMutex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogFreeRtosMutexStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogFreeRtosSysUpTime.c
)

target_include_directories(SolidSyslogFreeRtos INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogFreeRtosMutexPrivate.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h
)

target_link_libraries(SolidSyslogFreeRtos INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::FreeRtos ALIAS SolidSyslogFreeRtos)

solidsyslog_install_public_headers()
