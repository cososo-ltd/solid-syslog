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
