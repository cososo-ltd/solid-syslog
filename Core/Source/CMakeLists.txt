set(SOURCES
    SolidSyslog.c
    SolidSyslogError.c
    SolidSyslogConfigLock.c
    SolidSyslogSwitchingSender.c
    SolidSyslogBuffer.c
    SolidSyslogFormatter.c
    SolidSyslogMetaSd.c
    SolidSyslogNullBuffer.c
    SolidSyslogCircularBuffer.c
    SolidSyslogCircularBufferStatic.c
    SolidSyslogPoolAllocator.c
    SolidSyslogMutex.c
    SolidSyslogNullMutex.c
    SolidSyslogSender.c
    SolidSyslogStore.c
    SolidSyslogNullStore.c
    SolidSyslogNullSecurityPolicy.c
    SolidSyslogCrc16.c
    SolidSyslogCrc16Policy.c
    SolidSyslogStream.c
    SolidSyslogStructuredData.c
    SolidSyslogTimeQualitySd.c
    SolidSyslogOriginSd.c
    SolidSyslogFile.c
    SolidSyslogBlockStore.c
    RecordStore.c
    BlockSequence.c
    SolidSyslogBlockDevice.c
    SolidSyslogFileBlockDevice.c
    SolidSyslogUdpPayload.c
)

if(HAVE_ATOMIC_COUNTER)
    list(APPEND SOURCES SolidSyslogAtomicCounter.c)
endif()
# SolidSyslogAtomicCounter.c is the public vtable dispatcher — included
# under the same HAVE_ATOMIC_COUNTER gate as the platform impls
# (HAVE_STDATOMIC_H -> Platform/Atomics, HAVE_WINDOWS_INTERLOCKED ->
# Platform/Windows), so it's only compiled when at least one atomic
# backend is available.

if(SOLIDSYSLOG_POSIX OR SOLIDSYSLOG_WINSOCK OR DEFINED ENV{FREERTOS_KERNEL_PATH})
    list(APPEND SOURCES
        SolidSyslogResolver.c
        SolidSyslogDatagram.c
        SolidSyslogUdpSender.c
        SolidSyslogStreamSender.c
    )
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC  ${CMAKE_SOURCE_DIR}/Core/Interface
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PUBLIC SolidSyslogTunables)

if(SOLIDSYSLOG_POSIX)
    target_link_libraries(${PROJECT_NAME} PUBLIC rt)
endif()

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/Core/Interface/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
