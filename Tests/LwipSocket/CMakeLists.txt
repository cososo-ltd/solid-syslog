# Tests/LwipSocket - host-side unit tests for the lwIP Sockets API adapters
# under Platform/LwipSocket/Source/. Each test executable recompiles the adapter
# source under test against the LwipSocketFakes config + the real lwIP headers
# from $LWIP_PATH/src/include.
#
# A separate executable per class, and a separate directory from any other lwIP
# API tier, for the reason Tests/Lwip states: lwIP's opt.h has hundreds of
# #ifdef configurations, so folding these into the mega-test executable trips
# cppcheck's `toomanyconfigs` limit. The configs are also mutually exclusive -
# LWIP_SOCKET=1 and NO_SYS=0 cannot share a lwipopts.h with a NO_SYS=1 one.
#
# Gated by LWIP_PATH at the top-level (this CMakeLists is only entered when the
# env var is set).

set(LWIPSOCKET_TEST_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/Platform/LwipSocket/Interface
    ${CMAKE_SOURCE_DIR}/Platform/LwipSocket/Source
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests/Support
    ${CMAKE_SOURCE_DIR}/Tests/Support/LwipSocketFakes/Interface
    ${SOLIDSYSLOG_LWIP_PATH}/src/include
)

# lwIP upstream byte-order and address helpers. lwip_htons / lwip_htonl and
# ip4addr_aton are functions, not macros, unless the arch supplies its own, so a
# test exe that links the adapters needs them. Wrapped as a STATIC library with
# cppcheck disabled so the `toomanyconfigs` warning from opt.h's #ifdef forest
# does not fail our build; we do not audit upstream lwIP, that is lwIP's concern.
# Our own Platform/LwipSocket/Source/ files stay cppchecked because their
# target's default C_CPPCHECK is honoured.
#
# Built here against this tier's lwipopts.h rather than shared with another
# tier's, so the two configurations cannot leak into each other.
add_library(LwipSocketUpstreamHelpers STATIC
    ${SOLIDSYSLOG_LWIP_PATH}/src/core/def.c
    ${SOLIDSYSLOG_LWIP_PATH}/src/core/ipv4/ip4_addr.c
)
target_include_directories(LwipSocketUpstreamHelpers PRIVATE ${LWIPSOCKET_TEST_INCLUDE_DIRS})
set_target_properties(LwipSocketUpstreamHelpers PROPERTIES
    C_CPPCHECK ""
)

# ---- SolidSyslogLwipSocketAddressTest ----

add_executable(SolidSyslogLwipSocketAddressTest
    SolidSyslogLwipSocketAddressTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/LwipSocket/Source/SolidSyslogLwipSocketAddress.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipSocket/Source/SolidSyslogLwipSocketAddressStatic.c
)

target_link_libraries(SolidSyslogLwipSocketAddressTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    LwipSocketUpstreamHelpers
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogLwipSocketAddressTest PRIVATE ${LWIPSOCKET_TEST_INCLUDE_DIRS})

# Disable cppcheck on the C++ test sources only - they pull in lwIP's opt.h with
# too many #ifdef paths for cppcheck. The production .c files still get
# cppchecked via the default C_CPPCHECK inherited from CMAKE_C_CPPCHECK.
set_target_properties(SolidSyslogLwipSocketAddressTest PROPERTIES
    CXX_CPPCHECK ""
)

add_test(NAME SolidSyslogLwipSocketAddressTest COMMAND SolidSyslogLwipSocketAddressTest)

# ---- SolidSyslogLwipSocketResolverTest ----

add_executable(SolidSyslogLwipSocketResolverTest
    SolidSyslogLwipSocketResolverTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Tests/Support/LwipSocketFakes/Source/LwipNetdbFake.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipSocket/Source/SolidSyslogLwipSocketAddress.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipSocket/Source/SolidSyslogLwipSocketAddressStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipSocket/Source/SolidSyslogLwipSocketResolver.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipSocket/Source/SolidSyslogLwipSocketResolverStatic.c
)

target_link_libraries(SolidSyslogLwipSocketResolverTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    LwipSocketUpstreamHelpers
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogLwipSocketResolverTest PRIVATE ${LWIPSOCKET_TEST_INCLUDE_DIRS})

set_target_properties(SolidSyslogLwipSocketResolverTest PROPERTIES
    CXX_CPPCHECK ""
)

add_test(NAME SolidSyslogLwipSocketResolverTest COMMAND SolidSyslogLwipSocketResolverTest)

# ---- SolidSyslogLwipSocketDatagramTest ----

add_executable(SolidSyslogLwipSocketDatagramTest
    SolidSyslogLwipSocketDatagramTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Tests/Support/LwipSocketFakes/Source/LwipSocketsFake.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipSocket/Source/SolidSyslogLwipSocketAddress.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipSocket/Source/SolidSyslogLwipSocketAddressStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipSocket/Source/SolidSyslogLwipSocketDatagram.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipSocket/Source/SolidSyslogLwipSocketDatagramStatic.c
)

target_link_libraries(SolidSyslogLwipSocketDatagramTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    LwipSocketUpstreamHelpers
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogLwipSocketDatagramTest PRIVATE ${LWIPSOCKET_TEST_INCLUDE_DIRS})

set_target_properties(SolidSyslogLwipSocketDatagramTest PROPERTIES
    CXX_CPPCHECK ""
)

add_test(NAME SolidSyslogLwipSocketDatagramTest COMMAND SolidSyslogLwipSocketDatagramTest)
