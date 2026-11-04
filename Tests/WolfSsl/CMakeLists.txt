# Tests/WolfSsl - host-side unit tests for the wolfSSL adapter under
# Platform/WolfSsl/Source/. Each test executable recompiles the adapter source
# under test against the wolfSSL headers at $WOLFSSL_DIR and links the
# WolfSslFakes double in place of the library - same shape as Tests/MbedTls/.
#
# Gated by SOLIDSYSLOG_WOLFSSL at the top-level CMakeLists (this CMakeLists is
# only entered when the option is ON), which itself defaults ON when the
# WOLFSSL_DIR environment variable is set.

# cppcheck walks the wolfSSL headers these targets put on the include path and
# reports style findings inside them (C-style casts in wc_port.h, too many
# configurations). Vendor headers are not ours to fix, so cppcheck is disabled
# per target, as the Mbed TLS test executables do for the same reason.

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

set_target_properties(SolidSyslogWolfSslHmacSha256PolicyTest PROPERTIES
    C_CPPCHECK ""
    CXX_CPPCHECK ""
)

solidsyslog_add_test(SolidSyslogWolfSslHmacSha256PolicyTest)

# SolidSyslogWolfSslAesGcmPolicy - keyed at-rest AEAD (confidentiality +
# integrity). Driven against the WolfSslFake GCM capture double + RNG nonce
# source: the unit suite proves the adapter's wiring and every fallible-call
# error path; genuine AES-GCM round-trip / tamper / wrong-key correctness lives
# in Tests/WolfSslIntegration against the real library.
add_executable(SolidSyslogWolfSslAesGcmPolicyTest
    SolidSyslogWolfSslAesGcmPolicyTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslAesGcmPolicy.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslAesGcmPolicyGcm.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslAesGcmPolicyStatic.c
)

target_link_libraries(SolidSyslogWolfSslAesGcmPolicyTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    WolfSslFakes
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogWolfSslAesGcmPolicyTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Interface
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests
)

set_target_properties(SolidSyslogWolfSslAesGcmPolicyTest PROPERTIES
    C_CPPCHECK ""
    CXX_CPPCHECK ""
)

solidsyslog_add_test(SolidSyslogWolfSslAesGcmPolicyTest)

# SolidSyslogWolfSslNullCredentials - the credentials Null object. Its Install
# takes a WOLFSSL_CTX* it never dereferences, so this exe needs neither the
# fakes nor the upstream include path.
add_executable(SolidSyslogWolfSslNullCredentialsTest
    SolidSyslogWolfSslNullCredentialsTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslNullCredentials.c
)

target_link_libraries(SolidSyslogWolfSslNullCredentialsTest PRIVATE
    ${PROJECT_NAME}
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogWolfSslNullCredentialsTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Tests
)

solidsyslog_add_test(SolidSyslogWolfSslNullCredentialsTest)

# SolidSyslogWolfSslStream pool: Create / Destroy over the static pool, the
# config validation and the config-lock slot walk.
add_executable(SolidSyslogWolfSslStreamPoolTest
    SolidSyslogWolfSslStreamPoolTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslStream.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslStreamStatic.c
    ${CMAKE_SOURCE_DIR}/Tests/WolfSslCredentialsFake.c
)

target_link_libraries(SolidSyslogWolfSslStreamPoolTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    WolfSslFakes
    StreamFake
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogWolfSslStreamPoolTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Interface
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests
)

set_target_properties(SolidSyslogWolfSslStreamPoolTest PROPERTIES
    C_CPPCHECK ""
    CXX_CPPCHECK ""
)

solidsyslog_add_test(SolidSyslogWolfSslStreamPoolTest)

# SolidSyslogWolfSslStream: vtable behaviour driven against WolfSslTlsFake,
# the link-time double for the wolfSSL TLS API.
add_executable(SolidSyslogWolfSslStreamTest
    SolidSyslogWolfSslStreamTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslStream.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslStreamStatic.c
    ${CMAKE_SOURCE_DIR}/Tests/WolfSslCredentialsFake.c
)

target_link_libraries(SolidSyslogWolfSslStreamTest PRIVATE
    ${PROJECT_NAME}
    AddressFake
    ConfigLockFake
    ErrorHandlerFake
    WolfSslFakes
    StreamFake
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogWolfSslStreamTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Interface
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests
)

set_target_properties(SolidSyslogWolfSslStreamTest PROPERTIES
    C_CPPCHECK ""
    CXX_CPPCHECK ""
)

solidsyslog_add_test(SolidSyslogWolfSslStreamTest)

# SolidSyslogWolfSslPemBufferCredentials - the credentials backend that loads
# PEM from caller-owned buffers onto the context. Driven against WolfSslTlsFake,
# which captures what reached wolfSSL_CTX_load_verify_buffer and the client
# credential pair.
add_executable(SolidSyslogWolfSslPemBufferCredentialsTest
    SolidSyslogWolfSslPemBufferCredentialsTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslPemBufferCredentials.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslPemBufferCredentialsStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source/SolidSyslogWolfSslNullCredentials.c
)

target_link_libraries(SolidSyslogWolfSslPemBufferCredentialsTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    WolfSslFakes
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogWolfSslPemBufferCredentialsTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Interface
    ${CMAKE_SOURCE_DIR}/Platform/WolfSsl/Source
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests
)

set_target_properties(SolidSyslogWolfSslPemBufferCredentialsTest PROPERTIES
    C_CPPCHECK ""
    CXX_CPPCHECK ""
)

solidsyslog_add_test(SolidSyslogWolfSslPemBufferCredentialsTest)
