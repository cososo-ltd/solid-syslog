# FreeRTOS adapter pack. An INTERFACE library rather than a precompiled one: the
# integrator's FreeRTOSConfig.h changes the layout of TCBs and semaphores, so
# each consumer compiles these sources against their own config. Linking
# SolidSyslog::FreeRtos is enough — the pack carries its sources. The consumer
# points at their own FreeRTOS-Kernel tree.
#
# Posix, Windows and OpenSsl compile PRIVATE into ${PROJECT_NAME} instead; their
# ABIs are OS-level rather than header-configured.

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
