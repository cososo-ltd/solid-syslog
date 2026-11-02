# Tests/WolfSsl - host-side unit tests for the wolfSSL adapter under
# Platform/WolfSsl/Source/. Each test executable recompiles the adapter source
# under test against the wolfSSL headers at $WOLFSSL_DIR and links the
# WolfSslFakes double in place of the library - same shape as Tests/MbedTls/.
#
# Gated by SOLIDSYSLOG_WOLFSSL at the top-level CMakeLists (this CMakeLists is
# only entered when the option is ON), which itself defaults ON when the
# WOLFSSL_DIR environment variable is set.

# SolidSyslogWolfSslHmacSha256Policy - keyed at-rest integrity policy, driven
# against the deterministic wc_Hmac* double.
add_executable(SolidSyslogWolfSslHmacSha256PolicyTest
    SolidSyslogWolfSslHmacSha256PolicyTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslHmacSha256Policy.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslHmacSha256PolicyStatic.c
)

target_link_libraries(SolidSyslogWolfSslHmacSha256PolicyTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    WolfSslFakes
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogWolfSslHmacSha256PolicyTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Interface
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests
)

solidsyslog_add_test(SolidSyslogWolfSslHmacSha256PolicyTest)
