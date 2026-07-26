# Consumer smoke test (S30.04).
#
# A throwaway project that consumes SolidSyslog the way an integrator does, so
# "the library works as a CMake subproject" is something CI checks rather than
# something we assume. It is deliberately NOT part of the SolidSyslog build:
# nothing in the library references it, and it is configured standalone with the
# library pulled in underneath.
#
# FetchContent is the documented route and every other route (add_subdirectory,
# a submodule) reduces to the same nesting, so exercising this one covers them.
# FETCHCONTENT_SOURCE_DIR_SOLIDSYSLOG redirects it at the checkout under test —
# the real FetchContent code path, minus the clone, so a PR is validated against
# its own tree rather than whatever main happens to hold:
#
#   cmake -S ci/consumer-smoke -B <build> \
#         -DFETCHCONTENT_SOURCE_DIR_SOLIDSYSLOG=<repo root>
#
cmake_minimum_required(VERSION 3.16)
project(SolidSyslogConsumerSmoke C)

# A consumer with tests of their own calls this, and it sets BUILD_TESTING ON
# for the whole build. That used to switch SolidSyslog's unit suite on and
# demand CppUTest; SOLIDSYSLOG_BUILD_TESTING is why it no longer does.
include(CTest)

include(FetchContent)
FetchContent_Declare(SolidSyslog
    GIT_REPOSITORY https://github.com/cososo-ltd/solid-syslog.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(SolidSyslog)

# Nothing of ours belongs in a consumer's build graph. Naming the targets
# individually makes a regression say what leaked, rather than surfacing later
# as a mysterious CppUTest or linker-script failure in someone else's project.
foreach(_leaked IN ITEMS
    SolidSyslogTests
    BddTargetTests
    SolidSyslogBddTarget
    SolidSyslogBddTargetLwip
    manifest
    iwyu
)
    if(TARGET ${_leaked})
        message(FATAL_ERROR
            "SolidSyslog defined the top-level-only target '${_leaked}' inside a "
            "consumer's build — SOLIDSYSLOG_IS_TOP_LEVEL gating has regressed.")
    endif()
endforeach()

# The packs an embedded integrator links. They exist only when the upstream
# trees are on the environment, which the cross container provides; asserting
# them here catches a pack that stops being defined under nesting.
if(DEFINED ENV{LWIP_PATH} AND NOT TARGET SolidSyslog::LwipRaw)
    message(FATAL_ERROR "LWIP_PATH is set but SolidSyslog::LwipRaw was not defined.")
endif()
if(DEFINED ENV{FREERTOS_KERNEL_PATH} AND NOT TARGET SolidSyslog::FreeRtos)
    message(FATAL_ERROR "FREERTOS_KERNEL_PATH is set but SolidSyslog::FreeRtos was not defined.")
endif()

# Bare-metal cross builds have no linker script here, and proving we can link an
# ELF is not the point — the point is that the library compiles under someone
# else's toolchain. A static library gets there without the scaffolding.
if(CMAKE_CROSSCOMPILING)
    add_library(consumer_smoke STATIC main.c)
else()
    add_executable(consumer_smoke main.c)
endif()

target_link_libraries(consumer_smoke PRIVATE SolidSyslog)
