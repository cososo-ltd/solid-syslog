# LittleFS-backed integration tests: links real LittleFS rather than the fake in
# Tests/Support/LittleFsFakes, and drives the whole store stack - BlockStore ->
# FileBlockDevice -> SolidSyslogLittleFsFile -> lfs - over an emulated flash
# device that can lose power mid-write.
#
# Separate binary from SolidSyslogTests for the same reason MbedTlsIntegration
# is: the unit suite links the fake lfs_* symbols, and the two cannot share one
# executable without a fake/real collision.
#
# Gated by $LITTLEFS_PATH at the top-level CMakeLists.

set(LITTLEFS_SOURCE_DIR "${SOLIDSYSLOG_LITTLEFS_PATH}")
if(NOT EXISTS "${LITTLEFS_SOURCE_DIR}/lfs.c")
    message(FATAL_ERROR
        "Tests/LittleFsIntegration requires LITTLEFS_PATH to point at a LittleFS source tree; "
        "got LITTLEFS_PATH='${LITTLEFS_SOURCE_DIR}' which has no lfs.c.")
endif()

# Upstream LittleFS as its own library so our warning surface does not reach it.
# lfs.c raises ~135 diagnostics under the project's -Werror set, which is not
# upstream's problem to solve and not ours to pin a version over - the same
# posture Tests/MbedTlsIntegration takes. clang-tidy and cppcheck are disabled
# on it for the same reason.
add_library(littlefs_upstream STATIC
    ${LITTLEFS_SOURCE_DIR}/lfs.c
    ${LITTLEFS_SOURCE_DIR}/lfs_util.c
)

target_compile_options(littlefs_upstream PRIVATE -w)

target_include_directories(littlefs_upstream SYSTEM PUBLIC ${LITTLEFS_SOURCE_DIR})

set_target_properties(littlefs_upstream PROPERTIES
    C_CLANG_TIDY ""
    CXX_CLANG_TIDY ""
    C_CPPCHECK ""
    CXX_CPPCHECK ""
)

add_executable(LittleFsIntegrationTests
    main.cpp
    LittleFsDisk.c
    LittleFsDiskTest.cpp
    SolidSyslogLittleFsDurabilityTest.cpp
)

# The adapter arrives through its own umbrella rather than as a list of source
# paths, so the suite compiles exactly what a consumer links. A source added to
# the pack reaches this test without anyone remembering to add it here - which
# is the whole point of testing the real thing.
target_link_libraries(LittleFsIntegrationTests PRIVATE
    ${PROJECT_NAME}
    SolidSyslog::LittleFs
    littlefs_upstream
    CppUTest
    CppUTestExt
)

target_include_directories(LittleFsIntegrationTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests
    ${CMAKE_SOURCE_DIR}/Tests/Support
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME LittleFsIntegrationTests COMMAND LittleFsIntegrationTests)
