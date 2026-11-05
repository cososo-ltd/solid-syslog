add_library(ErrorHandlerFake STATIC ErrorHandlerFake.c)
target_include_directories(ErrorHandlerFake PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ErrorHandlerFake PUBLIC ${PROJECT_NAME})

add_library(BlockDeviceFake STATIC BlockDeviceFake.c)
target_include_directories(BlockDeviceFake PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(BlockDeviceFake PUBLIC ${PROJECT_NAME})

# The store bench drives a BlockStore over the BlockDeviceFake and records what
# happened, for the store sweep and the named regression tests alike. C++ with
# no CppUTest dependency, so both executables link it.
add_library(StoreBench STATIC StoreBench.cpp)
target_include_directories(StoreBench PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StoreBench PUBLIC ${PROJECT_NAME} BlockDeviceFake)

# The stream fake: a Stream whose Open, Send and Close are scripted and
# counted, and which keeps every byte it accepted so the frames a sender
# wrote can be read back. A library because the scenario suite links it too.
add_library(StreamFake STATIC StreamFake.c)
target_include_directories(StreamFake PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StreamFake PUBLIC ${PROJECT_NAME})

# The scenario bench runs the real Core end to end over the fakes above and
# records what came out, for the scenario suite and the named tests alike.
# C++ with no CppUTest dependency, so both executables link it.
add_library(ScenarioBench STATIC ScenarioBench.cpp SyslogFieldParser.cpp)
target_include_directories(ScenarioBench PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ScenarioBench PUBLIC ${PROJECT_NAME} AddressFake BlockDeviceFake CounterFake ResolverFake SecurityPolicyFake StreamFake)

# An opaque address token for tests whose stream never reads a sockaddr.
add_library(AddressFake STATIC AddressFake.c)
target_include_directories(AddressFake PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(AddressFake PUBLIC ${PROJECT_NAME})

# A Resolver that always answers, and a sequenceId counter with no platform
# behind it: what a stream sender and a meta SD need to run on any host.
add_library(ResolverFake STATIC ResolverFake.c)
target_include_directories(ResolverFake PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ResolverFake PUBLIC ${PROJECT_NAME})

add_library(CounterFake STATIC CounterFake.c)
target_include_directories(CounterFake PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(CounterFake PUBLIC ${PROJECT_NAME})

# A security policy that seals and opens as the Null policy does until told to
# fail as a keyed policy does, reporting each fault as one would: what the
# scenario bench's store needs to meet a withheld key, a failing backend or a
# record that fails verification.
add_library(SecurityPolicyFake STATIC SecurityPolicyFake.c)
target_include_directories(SecurityPolicyFake PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SecurityPolicyFake PUBLIC ${PROJECT_NAME})

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
        ${SOLIDSYSLOG_MBEDTLS_DIR}/include
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

# wolfSSL fake - same shape as MbedTlsFakes. Headers come from $WOLFSSL_DIR's
# install; we never link the real library into a target that pulls in this fake.
if(SOLIDSYSLOG_WOLFSSL)
    add_library(WolfSslFakes STATIC WolfSslFake.c WolfSslTlsFake.c)

    target_include_directories(WolfSslFakes PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SOLIDSYSLOG_WOLFSSL_DIR}/include
    )

    target_link_libraries(WolfSslFakes PUBLIC ${PROJECT_NAME})

    # cppcheck walks the wolfSSL headers and reports style findings inside
    # them; they are not ours to fix. Disabled per target, as for MbedTlsFakes.
    set_target_properties(WolfSslFakes PROPERTIES
        C_CPPCHECK ""
        CXX_CPPCHECK ""
    )
endif()
