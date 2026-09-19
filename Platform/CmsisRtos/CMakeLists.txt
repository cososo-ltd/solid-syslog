# CMSIS-RTOS2 adapter pack. An INTERFACE library rather than a precompiled one:
# cmsis_os2.h comes from the consumer's RTOS distribution rather than from us or
# from the system, so these sources compile in the CONSUMER's target against the
# header their kernel ships. Linking SolidSyslog::CmsisRtos is enough - the pack
# carries its sources.
#
# Unlike the other INTERFACE packs, this one needs no upstream tree for OUR own
# tests: cmsis_os2.h is pure declarations, so Tests/Support/CmsisRtosFakes
# supplies the API outright and the suite builds in the ordinary debug preset.
# That is why the registry row defaults it on rather than keying it to a path.

add_library(SolidSyslogCmsisRtos INTERFACE)

target_sources(SolidSyslogCmsisRtos INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogCmsisRtosMutex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogCmsisRtosMutexStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogCmsisRtosSysUpTime.c
)

target_include_directories(SolidSyslogCmsisRtos INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogCmsisRtosMutexPrivate.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h
)

target_link_libraries(SolidSyslogCmsisRtos INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::CmsisRtos ALIAS SolidSyslogCmsisRtos)

solidsyslog_install_public_headers()
