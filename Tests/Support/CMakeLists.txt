add_library(ErrorHandlerFake STATIC ErrorHandlerFake.c)
target_include_directories(ErrorHandlerFake PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ErrorHandlerFake PUBLIC ${PROJECT_NAME})

add_library(ConfigLockFake STATIC ConfigLockFake.c)
target_include_directories(ConfigLockFake PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ConfigLockFake PUBLIC ${PROJECT_NAME})

if(SOLIDSYSLOG_POSIX)
    add_library(PosixFakes STATIC
        ClockFake.c
        MqFake.c
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

# mbedTLS fake — same shape as OpenSslFakes. Headers come from $MBEDTLS_DIR;
# we never link upstream libmbedtls into a target that pulls in this fake.
if(SOLIDSYSLOG_MBEDTLS)
    add_library(MbedTlsFakes STATIC MbedTlsFake.c)

    target_include_directories(MbedTlsFakes PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        $ENV{MBEDTLS_DIR}/include
    )

    target_link_libraries(MbedTlsFakes PUBLIC ${PROJECT_NAME})

    # cppcheck cannot resolve mbedTLS's `#include MBEDTLS_USER_CONFIG_FILE`
    # macro indirection in build_info.h, so it errors on every TU that pulls
    # in <mbedtls/*.h>. The fake's whole point is to interpose mbedTLS
    # symbols — static-analysis value here is minimal. Disable per-target.
    set_target_properties(MbedTlsFakes PROPERTIES
        C_CPPCHECK ""
        CXX_CPPCHECK ""
    )
endif()
