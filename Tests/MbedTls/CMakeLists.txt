# Tests/MbedTls — host-side unit tests for the mbedTLS adapter under
# Platform/MbedTls/Source/. Each test executable recompiles the adapter
# source under test with the test's own (or upstream's) mbedtls_config.h
# on the include path and links against $MBEDTLS_DIR/include — same shape
# as Tests/FatFs/ and Tests/FreeRtos/, see project_header_configured_platforms.
#
# Gated by SOLIDSYSLOG_MBEDTLS at the top-level CMakeLists (this CMakeLists
# is only entered when the option is ON), which itself defaults ON when
# the MBEDTLS_DIR environment variable is set.

add_executable(SolidSyslogMbedTlsStreamPoolTest
    SolidSyslogMbedTlsStreamPoolTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/MbedTls/Source/SolidSyslogMbedTlsStream.c
    ${CMAKE_SOURCE_DIR}/Platform/MbedTls/Source/SolidSyslogMbedTlsStreamStatic.c
    ${CMAKE_SOURCE_DIR}/Tests/StreamFake.c
)

target_link_libraries(SolidSyslogMbedTlsStreamPoolTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    MbedTlsFakes
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogMbedTlsStreamPoolTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/MbedTls/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests
)

# cppcheck cannot resolve mbedTLS's `#include MBEDTLS_USER_CONFIG_FILE`
# macro indirection in build_info.h, so it errors on every TU that pulls
# in <mbedtls/*.h>. Both test exes pull mbedtls headers via the production
# adapter source and the MbedTlsFakes link; disable cppcheck per-target,
# same as MbedTlsIntegrationTests.
set_target_properties(SolidSyslogMbedTlsStreamPoolTest PROPERTIES
    C_CPPCHECK ""
    CXX_CPPCHECK ""
)

add_test(NAME SolidSyslogMbedTlsStreamPoolTest COMMAND SolidSyslogMbedTlsStreamPoolTest)

# Slice-2 onward: vtable behaviour driven against MbedTlsFake (link-time
# interposition of libmbedtls — separate binary from the integration test).
add_executable(SolidSyslogMbedTlsStreamTest
    SolidSyslogMbedTlsStreamTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/MbedTls/Source/SolidSyslogMbedTlsStream.c
    ${CMAKE_SOURCE_DIR}/Platform/MbedTls/Source/SolidSyslogMbedTlsStreamStatic.c
    ${CMAKE_SOURCE_DIR}/Tests/AddressFake.c
    ${CMAKE_SOURCE_DIR}/Tests/StreamFake.c
)

target_link_libraries(SolidSyslogMbedTlsStreamTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    MbedTlsFakes
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogMbedTlsStreamTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/MbedTls/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests
)

set_target_properties(SolidSyslogMbedTlsStreamTest PROPERTIES
    C_CPPCHECK ""
    CXX_CPPCHECK ""
)

add_test(NAME SolidSyslogMbedTlsStreamTest COMMAND SolidSyslogMbedTlsStreamTest)
