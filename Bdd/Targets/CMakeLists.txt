# BDD-driven target binaries — one per host platform. Each platform branch
# below produces an executable named `SolidSyslogBddTarget` (with the host's
# usual suffix: `.exe` on Windows, none on Linux). The same target name is
# reused across platforms because only one branch fires per configure.
#
# These are not pedagogical examples — they exist to drive the library
# under behave (Bdd/features/) and would normally be a CI artefact, not a
# user-facing entry point. The FreeRTOS counterpart lives under
# Bdd/Targets/FreeRtos/ and is reached from the top-level CMakeLists.txt
# via the cross-compile branch.

if(SOLIDSYSLOG_POSIX)
    # Linux BDD target: PosixMessageQueueBuffer + two pthreads + SwitchingSender
    # over UDP / TCP / TLS / mTLS (TLS only when SOLIDSYSLOG_OPENSSL).
    set(LINUX_SOURCES
        Linux/main.c
        Linux/BddTargetCommandLine.c
        Linux/BddTargetTcpConfig.c
        Linux/BddTargetUdpConfig.c
        Common/BddTargetAppName.c
        Common/BddTargetErrorText.c
        Common/BddTargetCustomSd.c
        Common/BddTargetInteractive.c
        Common/BddTargetIps.c
        Common/BddTargetKey.c
        Common/BddTargetLanguage.c
        Common/BddTargetMessageSettings.c
        Common/BddTargetServiceThread.c
        Common/BddTargetStderrErrorHandler.c
        Common/BddTargetStoreSettings.c
        Common/BddTargetSwitchConfig.c
    )
    # One crypto pack fills the at-rest policies and, where it has a stream,
    # the TLS sender. OpenSSL first, as the Linux lane has always run; a
    # configure that names WolfSsl without OpenSsl (the tunable-override-wolfssl
    # preset) builds the same target over the wolfSSL pack instead.
    if(SOLIDSYSLOG_OPENSSL)
        list(APPEND LINUX_SOURCES
            Common/BddTargetSecurityPolicy_OpenSsl.c
            Common/BddTargetTlsSender_OpenSsl_PosixTcp.c
            Common/BddTargetOpenSslCredentials.c
            Common/BddTargetTlsConfig.c
            Common/BddTargetMtlsConfig.c
        )
    elseif(SOLIDSYSLOG_WOLFSSL)
        # The TLS and mTLS `set` holders are pack-neutral; the sender over the
        # wolfSSL stream arrives with S34.02, until when TLS is unavailable.
        list(APPEND LINUX_SOURCES
            Common/BddTargetSecurityPolicy_WolfSsl.c
            Common/BddTargetTlsSender_Unavailable.c
            Common/BddTargetTlsConfig.c
            Common/BddTargetMtlsConfig.c
        )
    else()
        message(FATAL_ERROR
            "The Linux BDD target needs a crypto pack for its at-rest policies: "
            "select OpenSsl or WolfSsl.")
    endif()
    add_executable(SolidSyslogBddTarget ${LINUX_SOURCES})
    target_link_libraries(SolidSyslogBddTarget PRIVATE ${PROJECT_NAME} Threads::Threads)
    target_include_directories(SolidSyslogBddTarget PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Common
        ${CMAKE_CURRENT_SOURCE_DIR}/Linux
    )
    if(SOLIDSYSLOG_OPENSSL)
        target_link_libraries(SolidSyslogBddTarget PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    elseif(SOLIDSYSLOG_WOLFSSL)
        # The pack is an INTERFACE library compiled against the install at
        # WOLFSSL_DIR, which is also where the static library comes from.
        target_link_libraries(SolidSyslogBddTarget PRIVATE
            SolidSyslog::WolfSsl
            ${SOLIDSYSLOG_WOLFSSL_DIR}/lib/libwolfssl.a
            m
        )
        target_include_directories(SolidSyslogBddTarget PRIVATE ${SOLIDSYSLOG_WOLFSSL_DIR}/include)
    endif()
elseif(SOLIDSYSLOG_WINSOCK AND HAVE_WINDOWS_PLATFORM)
    # Windows BDD target: CircularBuffer + WindowsMutex driven by a Win32
    # service thread, mirroring the Linux target. Winsock UDP/TCP + Windows
    # clock/hostname/process-id.
    set(WINDOWS_SOURCES
        Windows/main.c
        Windows/BddTargetWindows.c
        Windows/BddTargetWindowsCommandLine.c
        Common/BddTargetAppName.c
        Common/BddTargetErrorText.c
        Common/BddTargetCustomSd.c
        Common/BddTargetInteractive.c
        Common/BddTargetIps.c
        Common/BddTargetKey.c
        Common/BddTargetLanguage.c
        Common/BddTargetMessageSettings.c
        Common/BddTargetServiceThread.c
        Common/BddTargetStderrErrorHandler.c
        Common/BddTargetStoreSettings.c
        Common/BddTargetSwitchConfig.c
    )
    if(SOLIDSYSLOG_OPENSSL)
        list(APPEND WINDOWS_SOURCES
            Common/BddTargetTlsSender_OpenSsl_WinsockTcp.c
            Common/BddTargetOpenSslCredentials.c
            Common/BddTargetTlsConfig.c
            Common/BddTargetMtlsConfig.c
        )
    else()
        list(APPEND WINDOWS_SOURCES Common/BddTargetTlsSender_Unavailable.c)
    endif()
    add_executable(SolidSyslogBddTarget ${WINDOWS_SOURCES})
    target_link_libraries(SolidSyslogBddTarget PRIVATE ${PROJECT_NAME})
    target_include_directories(SolidSyslogBddTarget PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Windows
        ${CMAKE_CURRENT_SOURCE_DIR}/Common
    )
    if(SOLIDSYSLOG_OPENSSL)
        target_link_libraries(SolidSyslogBddTarget PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
    # Merge the activeCodePage="UTF-8" manifest fragment into the linker's
    # auto-generated manifest so the MSVC CRT decodes argv as UTF-8.
    target_link_options(SolidSyslogBddTarget PRIVATE
        "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/Windows/BddTargetWindows.manifest"
    )
endif()
