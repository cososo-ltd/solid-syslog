if(SOLIDSYSLOG_POSIX)
    set(COMMON_SOURCES
        Common/ExampleAppName.c
        Common/ExampleCommandLine.c
        Common/ExampleIps.c
        Common/ExampleLanguage.c
        Common/ExampleServiceThread.c
        Common/ExampleStderrErrorHandler.c
        Common/ExampleUdpConfig.c
        Common/ExampleTcpConfig.c
        Common/ExampleInteractive.c
        Common/ExampleSwitchConfig.c
    )

    # Threaded example: PosixMessageQueueBuffer, two pthreads (requires POSIX threads and mqueue).
    # TLS transport is selected at link time via Common/ExampleTlsSender_<backend>.c
    # — OpenSSL+PosixTcp when SOLIDSYSLOG_OPENSSL, nil-object stub otherwise.
    # This is the only Linux example binary since S24.04 retired the
    # SingleTask + NullBuffer variant — its SwitchingSender + BlockStore
    # wiring is a strict superset of what SingleTask used to provide.
    set(THREADED_SOURCES Threaded/main.c ${COMMON_SOURCES})
    if(SOLIDSYSLOG_OPENSSL)
        list(APPEND THREADED_SOURCES
            Common/ExampleTlsSender_OpenSsl_PosixTcp.c
            Common/ExampleTlsConfig.c
            Common/ExampleMtlsConfig.c
        )
    else()
        list(APPEND THREADED_SOURCES Common/ExampleTlsSender_Unavailable.c)
    endif()
    add_executable(SolidSyslogThreadedExample ${THREADED_SOURCES})
    target_link_libraries(SolidSyslogThreadedExample PRIVATE ${PROJECT_NAME} Threads::Threads)
    target_include_directories(SolidSyslogThreadedExample PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Common
        ${CMAKE_CURRENT_SOURCE_DIR}/Threaded
    )
    if(SOLIDSYSLOG_OPENSSL)
        target_link_libraries(SolidSyslogThreadedExample PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
elseif(SOLIDSYSLOG_WINSOCK AND HAVE_WINDOWS_PLATFORM)
    # Windows example: CircularBuffer + WindowsMutex driven by a Win32 service
    # thread, mirroring the Linux Threaded example so BDD scenarios that
    # exercise the buffering path run on both runners. Winsock UDP/TCP +
    # Windows clock/hostname/process-id. Output binary is named
    # SolidSyslogExample.exe so BDD scenarios remain platform-agnostic.
    set(WINDOWS_EXAMPLE_SOURCES
        Windows/main.c
        Windows/SolidSyslogWindowsExample.c
        Windows/ExampleWindowsCommandLine.c
        Common/ExampleAppName.c
        Common/ExampleInteractive.c
        Common/ExampleIps.c
        Common/ExampleLanguage.c
        Common/ExampleServiceThread.c
        Common/ExampleStderrErrorHandler.c
        Common/ExampleSwitchConfig.c
    )
    # TLS transport: OpenSSL+WinsockTcp when SOLIDSYSLOG_OPENSSL, nil-object stub otherwise.
    if(SOLIDSYSLOG_OPENSSL)
        list(APPEND WINDOWS_EXAMPLE_SOURCES
            Common/ExampleTlsSender_OpenSsl_WinsockTcp.c
            Common/ExampleTlsConfig.c
            Common/ExampleMtlsConfig.c
        )
    else()
        list(APPEND WINDOWS_EXAMPLE_SOURCES Common/ExampleTlsSender_Unavailable.c)
    endif()
    add_executable(SolidSyslogWindowsExample ${WINDOWS_EXAMPLE_SOURCES})
    set_target_properties(SolidSyslogWindowsExample PROPERTIES OUTPUT_NAME "SolidSyslogExample")
    target_link_libraries(SolidSyslogWindowsExample PRIVATE ${PROJECT_NAME})
    target_include_directories(SolidSyslogWindowsExample PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Windows
        ${CMAKE_CURRENT_SOURCE_DIR}/Common
    )
    if(SOLIDSYSLOG_OPENSSL)
        target_link_libraries(SolidSyslogWindowsExample PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
    # Merge the activeCodePage="UTF-8" manifest fragment into the linker's
    # auto-generated manifest so the MSVC CRT decodes argv as UTF-8 (S13.17 #221).
    target_link_options(SolidSyslogWindowsExample PRIVATE
        "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/Windows/SolidSyslogWindowsExample.manifest"
    )
endif()
