# Tests/Lwip — host-side unit tests for the lwIP Raw API adapters under
# Platform/LwipRaw/Source/. Each test executable recompiles the adapter
# source under test against the LwipFakes config + the real lwIP headers
# from $LWIP_PATH/src/include.
#
# Why a separate executable per platform (vs. bundling into the main
# Tests/SolidSyslogTests):
#  - lwIP's opt.h has hundreds of #ifdef configurations; including it in
#    the mega-test executable trips cppcheck's `toomanyconfigs` limit.
#  - Cleaner cppcheck story: production .c files still get cppchecked
#    (default C_CPPCHECK property inherited), only the .cpp test files
#    skip cppcheck via CXX_CPPCHECK="". David's non-negotiable: platform
#    production code must be checked.
#  - Scales linearly: next OS (Zephyr, vxWorks) gets its own Tests/X/
#    mirroring this layout. See project_cmake_restructure_post_e28 memory.
#
# Gated by LWIP_PATH at the top-level (this CMakeLists is only entered
# when the env var is set).

set(LWIP_TEST_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Interface
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests/Support
    ${CMAKE_SOURCE_DIR}/Tests/Support/LwipFakes/Interface
    $ENV{LWIP_PATH}/src/include
)

# lwIP host-side fakes — recompiled into each test exe that needs them, mirroring
# the FreeRtosFakes pattern (sources live under Tests/Support/LwipFakes/Source/,
# pulled inline rather than wrapped as a static lib).
set(LWIP_FAKE_SOURCES
    ${CMAKE_SOURCE_DIR}/Tests/Support/LwipFakes/Source/LwipPbufFake.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/LwipFakes/Source/LwipUdpFake.c
)

# lwIP upstream parser. Wrapped as a STATIC library with cppcheck disabled
# so the `toomanyconfigs` warning from opt.h's #ifdef forest doesn't fail
# our build. We don't audit upstream lwIP code — that is lwIP's concern.
# Our own Platform/LwipRaw/Source/ files in the test executables below
# remain cppchecked because their target's default C_CPPCHECK is honoured.
add_library(LwipUpstreamParser STATIC
    $ENV{LWIP_PATH}/src/core/ipv4/ip4_addr.c
    $ENV{LWIP_PATH}/src/core/def.c
)
target_include_directories(LwipUpstreamParser PRIVATE ${LWIP_TEST_INCLUDE_DIRS})
set_target_properties(LwipUpstreamParser PROPERTIES
    C_CPPCHECK ""
)

# ---- SolidSyslogLwipRawAddressTest ----

add_executable(SolidSyslogLwipRawAddressTest
    SolidSyslogLwipRawAddressTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawAddress.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawAddressMessages.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawAddressStatic.c
)

target_link_libraries(SolidSyslogLwipRawAddressTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogLwipRawAddressTest PRIVATE ${LWIP_TEST_INCLUDE_DIRS})

# Disable cppcheck on the C++ test sources only — they pull in lwIP's
# opt.h with too many #ifdef paths for cppcheck. The production .c files
# above still get cppchecked via the default C_CPPCHECK inherited from
# CMAKE_C_CPPCHECK (set in the top-level CMakeLists when ENABLE_CPPCHECK).
set_target_properties(SolidSyslogLwipRawAddressTest PROPERTIES
    CXX_CPPCHECK ""
)

add_test(NAME SolidSyslogLwipRawAddressTest COMMAND SolidSyslogLwipRawAddressTest)

# ---- SolidSyslogLwipRawResolverTest ----

add_executable(SolidSyslogLwipRawResolverTest
    SolidSyslogLwipRawResolverTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawAddress.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawAddressMessages.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawAddressStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawResolver.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawResolverMessages.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawResolverStatic.c
)

target_link_libraries(SolidSyslogLwipRawResolverTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    LwipUpstreamParser
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogLwipRawResolverTest PRIVATE ${LWIP_TEST_INCLUDE_DIRS})

set_target_properties(SolidSyslogLwipRawResolverTest PROPERTIES
    CXX_CPPCHECK ""
)

add_test(NAME SolidSyslogLwipRawResolverTest COMMAND SolidSyslogLwipRawResolverTest)

# ---- SolidSyslogLwipRawDatagramTest ----

add_executable(SolidSyslogLwipRawDatagramTest
    SolidSyslogLwipRawDatagramTest.cpp
    main.cpp
    ${LWIP_FAKE_SOURCES}
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawAddress.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawAddressMessages.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawAddressStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawDatagram.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawDatagramMessages.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawDatagramStatic.c
)

target_link_libraries(SolidSyslogLwipRawDatagramTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogLwipRawDatagramTest PRIVATE ${LWIP_TEST_INCLUDE_DIRS})

set_target_properties(SolidSyslogLwipRawDatagramTest PROPERTIES
    CXX_CPPCHECK ""
)

add_test(NAME SolidSyslogLwipRawDatagramTest COMMAND SolidSyslogLwipRawDatagramTest)
