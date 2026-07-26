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
#
# The pack CARRIES its sources via target_sources(INTERFACE) (S30.02): a
# consumer that links SolidSyslog::PlusFat compiles the adapter into its own
# target, seeing its own FreeRTOSFATConfig.h — no per-file listing. The consumer
# still points at its own FreeRTOS-Plus-FAT tree and supplies its FF_Disk_t driver.

add_library(SolidSyslogPlusFat INTERFACE)

target_sources(SolidSyslogPlusFat INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogPlusFatFile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogPlusFatFileStatic.c
)

target_include_directories(SolidSyslogPlusFat INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogPlusFatFilePrivate.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h
)

target_link_libraries(SolidSyslogPlusFat INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::PlusFat ALIAS SolidSyslogPlusFat)

solidsyslog_install_public_headers()
