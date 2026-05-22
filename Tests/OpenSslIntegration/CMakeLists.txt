# OpenSSL-backed integration tests: links the real libssl and exercises
# SolidSyslogTlsStream end-to-end against an in-process server. Separate binary
# from SolidSyslogTests because unit tests link OpenSslFakes in place of libssl
# — the two cannot share one executable without fake/real symbol collision.
set(OPENSSL_INTEGRATION_TESTS_SOURCES
    BioPairStream.c
    TlsTestCert.c
    TlsTestServer.c
    SolidSyslogTlsStreamIntegrationTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Tests/AddressFake.c
)

# OpenSSL on MSVC requires applink.c when the application passes FILE* across
# runtime boundaries (e.g. PEM_write_*). vcpkg's OpenSSL find_package exposes
# the path via OPENSSL_APPLINK_SOURCE; non-MSVC builds don't need it. The
# vendored file trips /W4 (C4152 fn-ptr cast) and /WX-promoted C4996 on the
# CRT calls it makes, so suppress those warnings just for that source.
if(MSVC AND OPENSSL_APPLINK_SOURCE)
    list(APPEND OPENSSL_INTEGRATION_TESTS_SOURCES ${OPENSSL_APPLINK_SOURCE})
    set_source_files_properties(${OPENSSL_APPLINK_SOURCE} PROPERTIES
        COMPILE_OPTIONS "/wd4152;/wd4996"
    )
endif()

add_executable(OpenSslIntegrationTests ${OPENSSL_INTEGRATION_TESTS_SOURCES})

target_link_libraries(OpenSslIntegrationTests PRIVATE
    ${PROJECT_NAME}
    CppUTest
    CppUTestExt
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

target_include_directories(OpenSslIntegrationTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME OpenSslIntegrationTests COMMAND OpenSslIntegrationTests)
