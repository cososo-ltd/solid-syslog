add_library(ErrorHandlerFake STATIC ErrorHandlerFake.c)
target_include_directories(ErrorHandlerFake PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ErrorHandlerFake PUBLIC ${PROJECT_NAME})

if(SOLIDSYSLOG_POSIX)
    add_library(PosixFakes STATIC
        ClockFake.c
        SafeStringStandard.c
        SocketFake.c
    )

    target_include_directories(PosixFakes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(PosixFakes PUBLIC ${PROJECT_NAME})
endif()

if(SOLIDSYSLOG_WINSOCK)
    add_library(WinsockFakes STATIC WinsockFake.c SafeStringWindows.c)

    target_include_directories(WinsockFakes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(WinsockFakes PUBLIC ${PROJECT_NAME})
endif()

# OpenSSL fake is orthogonal to platform — link-time substitutes libssl symbols
# so the library's TLS code can be test-driven without real OpenSSL. Any target
# linking OpenSslFakes inherits the OpenSSL include path, which is needed
# explicitly on systems where OpenSSL isn't on the default compiler search
# path (e.g. Windows + vcpkg).
if(SOLIDSYSLOG_OPENSSL)
    add_library(OpenSslFakes STATIC OpenSslFake.c)

    target_include_directories(OpenSslFakes PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${OPENSSL_INCLUDE_DIR}
    )

    target_link_libraries(OpenSslFakes PUBLIC ${PROJECT_NAME})
endif()
