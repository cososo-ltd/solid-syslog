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

# config/ turns every gated adapter on; config-gates-off/ turns them all off.
set(SOLIDSYSLOG_SMOKE_CONFIG_DIR "config" CACHE STRING
    "Integrator config header directory, relative to this file.")

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

# The weak check, for the lanes that let the environment pick the packs: a pack
# that stops being defined under nesting is caught here. It cannot catch a pack
# that is absent BECAUSE the environment is bare — the condition and the bug
# have the same trigger — which is what SOLIDSYSLOG_SMOKE_LINK_PACKS below is
# for.
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

# The strong check (S30.05). Asserting a target exists only proves it was
# defined; the packs carry their sources into the consumer's target, so the
# thing that can actually break — do those sources compile and link against a
# consumer's own config header — is only exercised by linking them.
#
# The lane that drives this scrubs the environment first, so the packs can only
# come from -DSOLIDSYSLOG_PLATFORMS and the paths can only come from -D. That is
# the shape a real integrator has, and it is the shape no other lane here has:
# every one of them runs in a container that exports all five paths.
option(SOLIDSYSLOG_SMOKE_LINK_PACKS
    "Link the embedded packs rather than only asserting their targets exist" OFF)

if(SOLIDSYSLOG_SMOKE_LINK_PACKS)
    foreach(required_path IN ITEMS SOLIDSYSLOG_LWIP_PATH SOLIDSYSLOG_FREERTOS_KERNEL_PATH)
        if(NOT ${required_path})
            message(FATAL_ERROR
                "SOLIDSYSLOG_SMOKE_LINK_PACKS=ON needs ${required_path} — the consumer "
                "compiles the pack sources and must supply the upstream headers itself.")
        endif()
    endforeach()

    # The list is authoritative, so a platform we did not name must not be here.
    # Linking proves the named ones work; this proves the unnamed ones are
    # genuinely absent rather than arriving as a side effect of naming something
    # else — which is how PlusTcp used to reach a build that asked only for
    # FreeRtos (S30.06).
    foreach(_unnamed IN ITEMS PlusTcp MbedTls FatFs PlusFat)
        if(TARGET SolidSyslog::${_unnamed})
            message(FATAL_ERROR
                "SolidSyslog::${_unnamed} exists, but SOLIDSYSLOG_PLATFORMS did not name it "
                "(asked for: ${SOLIDSYSLOG_PLATFORMS}). The platform list is not authoritative.")
        endif()
    endforeach()

    target_link_libraries(consumer_smoke PRIVATE
        SolidSyslog::LwipRaw
        SolidSyslog::FreeRtos
    )

    # Exactly what an integrator supplies: their own config headers, then the
    # upstream trees. SolidSyslog's own include dirs arrive with the packs.
    target_include_directories(consumer_smoke PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/${SOLIDSYSLOG_SMOKE_CONFIG_DIR}
        ${SOLIDSYSLOG_LWIP_PATH}/src/include
        ${SOLIDSYSLOG_FREERTOS_KERNEL_PATH}/include
        ${SOLIDSYSLOG_FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM3
    )
endif()
