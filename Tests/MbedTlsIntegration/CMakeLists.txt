# mbedTLS-backed integration tests: links the real libmbedtls and (from
# slice 3 onward) drives an end-to-end handshake against an in-process TLS
# server. Separate binary from SolidSyslogTests because unit tests will
# link the future MbedTlsFake in place of libmbedtls — the two cannot share
# one executable without fake/real symbol collision.
#
# Builds mbedTLS from $MBEDTLS_DIR as a subproject. ENABLE_PROGRAMS is
# forced OFF here because we never want the upstream demo binaries (slow
# to build, no value to us). ENABLE_TESTING is left to the upstream default
# (OFF when added as a subproject).

set(MBEDTLS_SOURCE_DIR "$ENV{MBEDTLS_DIR}")
if(NOT EXISTS "${MBEDTLS_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "Tests/MbedTlsIntegration requires MBEDTLS_DIR to point at an mbedTLS source tree; "
        "got MBEDTLS_DIR='${MBEDTLS_SOURCE_DIR}' which has no CMakeLists.txt.")
endif()

# Don't pollute our top-level option namespace with mbedTLS's options.
# MBEDTLS_FATAL_WARNINGS is force-OFF: upstream sources don't (and won't)
# pass our strict -Werror surface, and treating them as our responsibility
# would tie our build to an mbedTLS version. Same posture as FreeRTOS-Kernel,
# Plus-TCP, and FatFs — see project_header_configured_platforms.
set(ENABLE_PROGRAMS OFF CACHE BOOL "Disable mbedTLS demo programs in our build" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "Disable mbedTLS upstream tests in our build" FORCE)
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "Upstream mbedTLS warnings are not errors in our build" FORCE)
add_subdirectory(${MBEDTLS_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/_mbedtls EXCLUDE_FROM_ALL)

# Belt-and-braces: our parent CMAKE_C_FLAGS adds -Werror=sign-conversion etc.
# that flow into add_subdirectory targets. Drop all warnings on the upstream
# libraries so a future mbedTLS bump doesn't break our build for unrelated
# diagnostic reasons. Same for clang-tidy: the parent build sets
# CMAKE_<LANG>_CLANG_TIDY under the `tidy` preset, which would apply our
# .clang-tidy rules to upstream code. Disable per-target.
foreach(_mbedtls_upstream_target IN ITEMS mbedtls mbedx509 mbedcrypto everest p256m)
    if(TARGET ${_mbedtls_upstream_target})
        target_compile_options(${_mbedtls_upstream_target} PRIVATE -w)
        set_target_properties(${_mbedtls_upstream_target} PROPERTIES
            C_CLANG_TIDY ""
            CXX_CLANG_TIDY ""
            C_CPPCHECK ""
            CXX_CPPCHECK ""
        )
    endif()
endforeach()

add_executable(MbedTlsIntegrationTests
    main.cpp
    SolidSyslogMbedTlsStreamIntegrationTest.cpp
    SocketStream.c
    MbedTlsTestCert.c
    MbedTlsTestServer.c
    ${CMAKE_SOURCE_DIR}/Tests/AddressFake.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/SafeStringStandard.c
    ${CMAKE_SOURCE_DIR}/Platform/MbedTls/Source/SolidSyslogMbedTlsStream.c
    ${CMAKE_SOURCE_DIR}/Platform/MbedTls/Source/SolidSyslogMbedTlsStreamStatic.c
)

target_link_libraries(MbedTlsIntegrationTests PRIVATE
    ${PROJECT_NAME}
    CppUTest
    CppUTestExt
    mbedtls
    mbedx509
    mbedcrypto
    Threads::Threads
)

target_include_directories(MbedTlsIntegrationTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/MbedTls/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests
    ${CMAKE_SOURCE_DIR}/Tests/Support
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# cppcheck on this target trips on `#include <mbedtls/version.h>` because
# mbedTLS's build_info.h uses `#include MBEDTLS_USER_CONFIG_FILE` (a macro)
# which cppcheck's preprocessor cannot resolve. The integration test's whole
# purpose is to exercise real libmbedtls end-to-end, so the static-analysis
# value on this TU is low — disable cppcheck on the target.
set_target_properties(MbedTlsIntegrationTests PROPERTIES
    C_CPPCHECK ""
    CXX_CPPCHECK ""
)

add_test(NAME MbedTlsIntegrationTests COMMAND MbedTlsIntegrationTests)
