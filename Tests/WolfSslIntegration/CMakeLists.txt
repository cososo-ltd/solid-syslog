# wolfSSL-backed integration tests: link the real libwolfssl and prove the
# cryptography the unit suites only wire. Separate binaries from
# SolidSyslogTests because the unit tests link WolfSslFakes in place of the
# library - the two cannot share one executable without fake/real symbol
# collision.
#
# Built twice, because the pack compiles against whatever wolfSSL build the
# integrator has: once against $WOLFSSL_DIR (the build with the options the
# pack asks for) and once against $WOLFSSL_BASELINE_DIR (a bare ./configure).
# Both are static installs from the same release, so each executable takes its
# headers and its library from one prefix and nothing else.

set(WOLFSSL_INTEGRATION_SOURCES
    main.cpp
    SolidSyslogWolfSslHmacSha256PolicyIntegrationTest.cpp
    SolidSyslogWolfSslAesGcmPolicyIntegrationTest.cpp
    SolidSyslogWolfSslStreamIntegrationTest.cpp
    WolfSslTestRng.c
    WolfSslTestServer.c
    WolfSslTestLibrary.c
    WolfSslTestMaterial.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/SocketStream.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslHmacSha256Policy.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslHmacSha256PolicyStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslAesGcmPolicy.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslAesGcmPolicyGcm.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslAesGcmPolicyStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslStream.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslStreamStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslNullCredentials.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslPemBufferCredentials.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslPemBufferCredentialsStatic.c
)

# The certificates the stream tests present. Neither wolfSSL build can issue
# one, so the OpenSSL command-line tool makes them at configure time, into the
# build tree: a validity window that has closed or not yet opened needs a
# generator that can back-date, and committing keys is avoided. The suite
# reads them by path from WOLFSSL_TEST_MATERIAL_DIR.
set(WOLFSSL_TEST_MATERIAL_DIR "${CMAKE_CURRENT_BINARY_DIR}/material")
add_custom_command(
    OUTPUT "${WOLFSSL_TEST_MATERIAL_DIR}/generated.stamp"
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/material/generate.sh" "${WOLFSSL_TEST_MATERIAL_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/material/generate.sh"
    COMMENT "Generating the wolfSSL integration test certificates"
    VERBATIM
)
add_custom_target(WolfSslTestMaterial DEPENDS "${WOLFSSL_TEST_MATERIAL_DIR}/generated.stamp")

# One executable per wolfSSL install. `prefix` holds include/ and lib/.
function(solidsyslog_add_wolfssl_integration target prefix)
    if(NOT EXISTS "${prefix}/include/wolfssl/options.h")
        message(FATAL_ERROR
            "Tests/WolfSslIntegration: ${target} needs an installed wolfSSL at '${prefix}' "
            "(include/wolfssl/options.h and lib/libwolfssl.a).")
    endif()
    add_executable(${target} ${WOLFSSL_INTEGRATION_SOURCES})
    add_dependencies(${target} WolfSslTestMaterial)
    target_compile_definitions(${target} PRIVATE
        WOLFSSL_TEST_MATERIAL_DIR="${WOLFSSL_TEST_MATERIAL_DIR}"
    )
    target_link_libraries(${target} PRIVATE
        ${PROJECT_NAME}
        AddressFake
        CppUTest
        CppUTestExt
        ${prefix}/lib/libwolfssl.a
        m
        Threads::Threads
    )
    target_include_directories(${target} PRIVATE
        ${prefix}/include
        ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Interface
        ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source
        ${CMAKE_SOURCE_DIR}/Core/Interface
        ${CMAKE_SOURCE_DIR}/Core/Source
        ${CMAKE_SOURCE_DIR}/Tests
        ${CMAKE_SOURCE_DIR}/Tests/Support
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    # cppcheck walks the wolfSSL headers and reports style findings inside
    # them; they are not ours to fix. Disabled per target, as for the Mbed TLS
    # integration executable.
    set_target_properties(${target} PROPERTIES
        C_CPPCHECK ""
        CXX_CPPCHECK ""
    )
    solidsyslog_add_test(${target})
endfunction()

solidsyslog_add_wolfssl_integration(WolfSslIntegrationTests "${SOLIDSYSLOG_WOLFSSL_DIR}")
if(SOLIDSYSLOG_WOLFSSL_BASELINE_DIR)
    solidsyslog_add_wolfssl_integration(WolfSslBaselineIntegrationTests "${SOLIDSYSLOG_WOLFSSL_BASELINE_DIR}")
endif()
