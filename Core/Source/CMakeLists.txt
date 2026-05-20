set(SOURCES
    SolidSyslog.c
    SolidSyslogError.c
    SolidSyslogConfigLock.c
    SolidSyslogSwitchingSender.c
    SolidSyslogSwitchingSenderStatic.c
    SolidSyslogBuffer.c
    SolidSyslogFormatter.c
    SolidSyslogMetaSd.c
    SolidSyslogMetaSdStatic.c
    SolidSyslogPassthroughBuffer.c
    SolidSyslogPassthroughBufferStatic.c
    SolidSyslogCircularBuffer.c
    SolidSyslogCircularBufferStatic.c
    SolidSyslogPoolAllocator.c
    SolidSyslogMutex.c
    SolidSyslogNullMutex.c
    SolidSyslogSender.c
    SolidSyslogNullSender.c
    SolidSyslogStore.c
    SolidSyslogNullSd.c
    SolidSyslogNullStore.c
    SolidSyslogNullBuffer.c
    SolidSyslogNullBlockDevice.c
    SolidSyslogNullSecurityPolicy.c
    SolidSyslogCrc16.c
    SolidSyslogCrc16Policy.c
    SolidSyslogStream.c
    SolidSyslogNullStream.c
    SolidSyslogStructuredData.c
    SolidSyslogTimeQualitySd.c
    SolidSyslogTimeQualitySdStatic.c
    SolidSyslogOriginSd.c
    SolidSyslogOriginSdStatic.c
    SolidSyslogFile.c
    SolidSyslogNullFile.c
    SolidSyslogBlockStore.c
    SolidSyslogBlockStoreStatic.c
    RecordStore.c
    RecordStoreStatic.c
    BlockSequence.c
    BlockSequenceStatic.c
    SolidSyslogBlockDevice.c
    SolidSyslogFileBlockDevice.c
    SolidSyslogFileBlockDeviceStatic.c
    SolidSyslogUdpPayload.c
)

if(HAVE_ATOMIC_COUNTER)
    list(APPEND SOURCES
        SolidSyslogAtomicCounter.c
        SolidSyslogNullAtomicCounter.c
    )
endif()
# SolidSyslogAtomicCounter.c is the public vtable dispatcher — included
# under the same HAVE_ATOMIC_COUNTER gate as the platform impls
# (HAVE_STDATOMIC_H -> Platform/Atomics, HAVE_WINDOWS_INTERLOCKED ->
# Platform/Windows), so it's only compiled when at least one atomic
# backend is available. SolidSyslogNullAtomicCounter.c is the shared
# pool-exhaustion fallback for both backends.

if(SOLIDSYSLOG_POSIX OR SOLIDSYSLOG_WINSOCK OR DEFINED ENV{FREERTOS_KERNEL_PATH})
    list(APPEND SOURCES
        SolidSyslogResolver.c
        SolidSyslogNullResolver.c
        SolidSyslogDatagram.c
        SolidSyslogNullDatagram.c
        SolidSyslogUdpSender.c
        SolidSyslogUdpSenderStatic.c
        SolidSyslogStreamSender.c
        SolidSyslogStreamSenderStatic.c
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
