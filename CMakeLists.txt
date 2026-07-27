cmake_minimum_required(VERSION 3.16)
project(SolidSyslog C CXX)

# Is SolidSyslog the top-level project, or is it nested inside a consumer's build
# via FetchContent / add_subdirectory / a submodule? Everything a consumer has no
# use for — the developer checks, the unit tests, the BDD targets, install() —
# hangs off this. PROJECT_IS_TOP_LEVEL says exactly this, but it needs CMake 3.21
# and the floor above is 3.16; raising the floor is consumer-visible, so compute
# it instead. This is CMake's own definition of that variable.
if(CMAKE_SOURCE_DIR STREQUAL SolidSyslog_SOURCE_DIR)
    set(SOLIDSYSLOG_IS_TOP_LEVEL TRUE)
else()
    set(SOLIDSYSLOG_IS_TOP_LEVEL FALSE)
endif()

# Every Platform pack installs its Interface/ headers the same way, and every one
# of those rules is top-level-only: the library ships no export set, so a nested
# build has nothing to contribute to the consumer's `install` target beyond
# scattering headers into their prefix. Called from each pack in place of ten
# copies of the same rule. CMAKE_CURRENT_SOURCE_DIR inside a function is still
# the calling directory, so each pack installs its own headers.
function(solidsyslog_install_public_headers)
    if(SOLIDSYSLOG_IS_TOP_LEVEL)
        include(GNUInstallDirs)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Interface/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h"
        )
    endif()
endfunction()

# Everything that ships (this file, Core/, Platform/*, cmake/*.cmake) addresses
# the library tree through SolidSyslog_SOURCE_DIR / SolidSyslog_BINARY_DIR, never
# CMAKE_SOURCE_DIR / CMAKE_BINARY_DIR. Under FetchContent or add_subdirectory the
# CMAKE_ forms point at the CONSUMER's tree, so the first thing that used one —
# the module path below — took the whole configure down. The project()-scoped
# names are correct at any nesting depth. Tests/ and Bdd/ only ever configure
# top-level and keep the CMAKE_ forms.
#
# LayerGuard's layering scan, the IWYU targets and the manifest generator are
# maintainer tooling: a consumer gets neither the cache entries nor the targets.
if(SOLIDSYSLOG_IS_TOP_LEVEL)
    list(APPEND CMAKE_MODULE_PATH "${SolidSyslog_SOURCE_DIR}/cmake")
    include(LayerGuard)
    solidsyslog_enforce_layering()
    include(Iwyu)
    include(Manifest)
endif()

# Default to C11 (for the optional C11 atomics counter), but let an externally
# supplied standard win so the pre-release C99 portability check can build the
# library at -std=c99 without editing this file. See docs/local-checks.md.
if(NOT DEFINED CMAKE_C_STANDARD)
    set(CMAKE_C_STANDARD 11)
endif()
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Our unit suite, under our own name. BUILD_TESTING belongs to CTest: a consumer
# who calls include(CTest) for their own tests sets it globally, and would then
# be asked for CppUTest to build a dependency's test suite. Library-only
# configurations (e.g. the `c99` portability preset) turn this off so they
# configure without CppUTest present; nested builds default it off.
#
# CppUTest is only needed when the suite is built. Threads is needed more widely
# (the host BDD targets link it), so it stays outside the suite guard — but both
# are still ours alone: nothing in Core/ or Platform/ links either, so neither
# find_package runs in a consumer's build.
option(SOLIDSYSLOG_BUILD_TESTING "Build the SolidSyslog unit test suite" ${SOLIDSYSLOG_IS_TOP_LEVEL})

# Tests/ addresses the tree through CMAKE_SOURCE_DIR and links the CppUTest
# targets found below, so it only ever works top-level. Turning the flag on
# inside someone else's build cannot succeed — say that here rather than failing
# further down on an undefined CppUTest target.
if(SOLIDSYSLOG_BUILD_TESTING AND NOT SOLIDSYSLOG_IS_TOP_LEVEL)
    message(WARNING
        "SOLIDSYSLOG_BUILD_TESTING is ignored here: the SolidSyslog unit suite "
        "only configures when SolidSyslog is the top-level project.")
    set(SOLIDSYSLOG_BUILD_TESTING OFF)
endif()

if(SOLIDSYSLOG_IS_TOP_LEVEL AND NOT CMAKE_CROSSCOMPILING)
    find_package(Threads REQUIRED)

    if(SOLIDSYSLOG_BUILD_TESTING)
        find_package(CppUTest REQUIRED)
    endif()
endif()

# Warning bar and developer instrumentation — top-level only. A consumer
# building libSolidSyslog.a inside their tree should not inherit -Werror (the
# bar is tuned against the compilers CI runs, not theirs), and SANITIZE /
# COVERAGE / ENABLE_CLANG_TIDY / ENABLE_CPPCHECK are maintainer switches whose
# generic names would collide with a consumer's own cache entries.
if(SOLIDSYSLOG_IS_TOP_LEVEL)
    if(MSVC)
        # /wd4200: char buffer[] (C99 flexible array member) — valid C99, MSVC warns as nonstandard extension
        # /wd4297: test infrastructure throws C++ exceptions from extern "C" functions
        # to propagate assertion failures from C fakes through CppUTest. Permanent suppression.
        add_compile_options(/W4 /WX /wd4200 /wd4297)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror)
    endif()

    option(SANITIZE "Enable AddressSanitizer and UndefinedBehaviourSanitizer" OFF)
    if(SANITIZE)
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()

    option(COVERAGE "Enable code coverage instrumentation" OFF)
    if(COVERAGE)
        add_compile_options(--coverage)
        add_link_options(--coverage)
    endif()

    option(ENABLE_CLANG_TIDY "Run clang-tidy on all source files during build" OFF)
    if(ENABLE_CLANG_TIDY)
        find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
        set(CMAKE_C_CLANG_TIDY   "${CLANG_TIDY_EXECUTABLE}")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}")
    endif()

    option(ENABLE_CPPCHECK "Run cppcheck on all source files during build" OFF)
    if(ENABLE_CPPCHECK)
        find_program(CPPCHECK_EXECUTABLE NAMES cppcheck)
        if(CPPCHECK_EXECUTABLE)
            set(CMAKE_C_CPPCHECK
                "${CPPCHECK_EXECUTABLE}"
                "--enable=warning,style,performance,portability"
                "--error-exitcode=1"
                "--suppress=missingIncludeSystem"
                "--suppressions-list=${SolidSyslog_SOURCE_DIR}/cppcheck_suppressions_tests.txt"
                "--inline-suppr"
                "--std=c11"
            )
            set(CMAKE_CXX_CPPCHECK
                "${CPPCHECK_EXECUTABLE}"
                "--enable=warning,style,performance,portability"
                "--error-exitcode=1"
                "--suppress=missingIncludeSystem"
                "--suppress=useStlAlgorithm"
                "--suppressions-list=${SolidSyslog_SOURCE_DIR}/cppcheck_suppressions_tests.txt"
                "--inline-suppr"
                "--std=c++17"
            )
        else()
            message(WARNING "cppcheck not found — ENABLE_CPPCHECK has no effect")
        endif()
    endif()
endif()

# Availability probes. These answer "could this platform build here?", nothing
# more — selection is the platform registry's job further down. Keeping the two
# apart is what lets one vocabulary cover auto-detected host platforms and
# explicitly declared embedded ones (S30.06).
include(CheckSymbolExists)
check_symbol_exists(sendto "sys/socket.h" HAVE_POSIX_SOCKETS)

set(_SOLIDSYSLOG_PREV_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
set(CMAKE_REQUIRED_LIBRARIES ws2_32)
check_symbol_exists(sendto "winsock2.h" HAVE_WINSOCK)
set(CMAKE_REQUIRED_LIBRARIES "${_SOLIDSYSLOG_PREV_CMAKE_REQUIRED_LIBRARIES}")
unset(_SOLIDSYSLOG_PREV_CMAKE_REQUIRED_LIBRARIES)

include(CheckCSourceCompiles)
# Detect real C11 atomics by *compiling* an _Atomic snippet rather than probing
# for <stdatomic.h>, which exists on every toolchain regardless of whether the
# _Atomic keyword is actually supported. The probe fails on a toolchain without
# working C11 atomics, excluding Platform/Atomics in favour of the Windows or
# Null counter. (The C99 portability preset declares HAVE_STDATOMIC_H=OFF up
# front — see docs/local-checks.md — so a C99 target skips the C11 atomics by
# design rather than relying on the probe to infer their absence.)
check_c_source_compiles(
    "#include <stdatomic.h>
#include <stdint.h>
int main(void)
{
    _Atomic uint32_t value;
    uint32_t expected = 0u;
    atomic_init(&value, 0u);
    (void) atomic_compare_exchange_strong_explicit(
        &value, &expected, 1u, memory_order_relaxed, memory_order_relaxed);
    return (int) atomic_load_explicit(&value, memory_order_relaxed);
}
"
    HAVE_STDATOMIC_H
)
check_symbol_exists(InterlockedIncrement "windows.h" HAVE_WINDOWS_INTERLOCKED)
check_symbol_exists(GetSystemTimeAsFileTime "windows.h" HAVE_WINDOWS_PLATFORM)

find_package(OpenSSL QUIET)

# Platform/Windows is one directory holding three separately-probed groups: the
# Interlocked counter, the Win32 clock/file/identity set, and the Winsock
# network adapters. Any one of them being available is reason to offer the
# platform; which parts compile is decided inside Platform/Windows.
if(HAVE_WINDOWS_INTERLOCKED OR HAVE_WINDOWS_PLATFORM OR HAVE_WINSOCK)
    set(HAVE_WINDOWS_ANY TRUE)
else()
    set(HAVE_WINDOWS_ANY FALSE)
endif()

# ─── Platform registry (S30.06) ─────────────────────────────────────────────
#
# One vocabulary for every platform SolidSyslog can build against. A consumer
# names the ones they want and links what those provide:
#
#     set(SOLIDSYSLOG_PLATFORMS "FreeRtos;LwipRaw;MbedTls;FatFs")
#
# Leave it unset and each platform falls back to its own availability default —
# a compile probe for the host ones, "the upstream path is on the environment"
# for the embedded ones — which is how every preset, devcontainer service and CI
# lane here works, so all of them configure exactly as before.
#
# Platforms attach to the build in one of two ways, and the rule is causal
# rather than arbitrary:
#
#   * Header-configured upstreams (lwIP, FreeRTOS, Plus-TCP, Plus-FAT, mbedTLS,
#     FatFs) cannot be precompiled — lwipopts.h, FreeRTOSConfig.h,
#     mbedtls_config.h and ffconf.h change layout and behaviour, and we cannot
#     see the consumer's copy. Each ships as an INTERFACE library carrying its
#     adapter sources, which compile into the CONSUMER's target against the
#     CONSUMER's config. These are the SolidSyslog::<Token> targets to link, and
#     it is the consumer's own target that supplies the upstream include paths.
#
#   * Stable system APIs (POSIX, Win32/Winsock, OpenSSL, C11 atomics) have no
#     such hazard, so they compile straight into libSolidSyslog.a. There is
#     nothing extra to link — SolidSyslog alone carries them.
#
# In one line: if the upstream needs your config header, you link it; otherwise
# it is already inside.
#
# Platforms are independent — no Platform/ directory references another — so
# naming any combination is legal, including two that fill the same role. The
# report below names roles precisely so an unintended combination is visible
# (see docs/NAMING.md on role-named pool tunables sizing both implementations).

# `kind` carries the distinction the two attachment modes turn on, and it is the
# reason the default column means different things per row:
#
#   probe    — a capability of this toolchain or host. The default IS the
#              answer, so naming one that probed false is an error: no amount of
#              configuration will make <stdatomic.h> work on a C99 compiler.
#              These compile into libSolidSyslog.a.
#   upstream — a header-configured upstream. The default is only a hint (we
#              noticed the tree on your environment); the platform itself needs
#              no path, because its sources compile in the consumer's target
#              against the consumer's config. Naming one is ALWAYS legal — a
#              consumer who vendors lwIP and exports nothing is the case S30.05
#              exists to serve. These are the SolidSyslog::<Token> link targets.
#
# Rows are split on `|` and read by index, so every field must be a token that
# cannot contain a separator. The upstream hints are collapsed to booleans here
# rather than interpolated as paths: a tree at a path containing `;` or `|`
# would otherwise shift every later field, and a mis-read `kind` turns the
# probe fatal-error below into a silent no-op. The default column is only ever
# used as a truth value, so nothing is lost.
foreach(upstream_hint MBEDTLS_DIR LWIP_PATH FREERTOS_PLUS_TCP_PATH
                      FREERTOS_KERNEL_PATH FATFS_PATH FREERTOS_PLUS_FAT_PATH)
    if(DEFINED ENV{${upstream_hint}} AND NOT "$ENV{${upstream_hint}}" STREQUAL "")
        set(HAVE_${upstream_hint} TRUE)
    else()
        set(HAVE_${upstream_hint} FALSE)
    endif()
endforeach()

#                                 token     option                default                        kind      directory            roles
set(SOLIDSYSLOG_PLATFORM_REGISTRY
    "Posix|SOLIDSYSLOG_POSIX|${HAVE_POSIX_SOCKETS}|probe|Platform/Posix|network file mutex clock"
    "Windows|SOLIDSYSLOG_WINDOWS|${HAVE_WINDOWS_ANY}|probe|Platform/Windows|network file mutex atomics clock"
    "Atomics|SOLIDSYSLOG_ATOMICS|${HAVE_STDATOMIC_H}|probe|Platform/Atomics|atomics"
    "OpenSsl|SOLIDSYSLOG_OPENSSL|${OpenSSL_FOUND}|probe|Platform/OpenSsl|tls"
    "MbedTls|SOLIDSYSLOG_MBEDTLS|${HAVE_MBEDTLS_DIR}|upstream|Platform/MbedTls|tls"
    "LwipRaw|SOLIDSYSLOG_LWIP|${HAVE_LWIP_PATH}|upstream|Platform/LwipRaw|network"
    "PlusTcp|SOLIDSYSLOG_PLUSTCP|${HAVE_FREERTOS_PLUS_TCP_PATH}|upstream|Platform/PlusTcp|network"
    "FreeRtos|SOLIDSYSLOG_FREERTOS|${HAVE_FREERTOS_KERNEL_PATH}|upstream|Platform/FreeRtos|mutex clock"
    "FatFs|SOLIDSYSLOG_FATFS|${HAVE_FATFS_PATH}|upstream|Platform/FatFs|file"
    "PlusFat|SOLIDSYSLOG_PLUSFAT|${HAVE_FREERTOS_PLUS_FAT_PATH}|upstream|Platform/PlusFat|file"
)

# Split one registry row into the names the loops below use. A row is
# `token|option|default|kind|directory|roles`.
macro(solidsyslog_read_platform row)
    string(REPLACE "|" ";" solidsyslog_row "${row}")
    list(GET solidsyslog_row 0 platform_token)
    list(GET solidsyslog_row 1 platform_option)
    list(GET solidsyslog_row 2 platform_default)
    list(GET solidsyslog_row 3 platform_kind)
    list(GET solidsyslog_row 4 platform_directory)
    list(GET solidsyslog_row 5 platform_roles)
endmacro()

set(SOLIDSYSLOG_PLATFORM_TOKENS "")
foreach(row IN LISTS SOLIDSYSLOG_PLATFORM_REGISTRY)
    solidsyslog_read_platform("${row}")
    list(APPEND SOLIDSYSLOG_PLATFORM_TOKENS ${platform_token})
    if(platform_default)
        option(${platform_option} "Include the ${platform_token} platform" ON)
    else()
        option(${platform_option} "Include the ${platform_token} platform" OFF)
    endif()
endforeach()

set(SOLIDSYSLOG_PLATFORMS "" CACHE STRING
    "Platforms to build, e.g. FreeRtos;LwipRaw;MbedTls. Empty selects each platform from its own availability.")

# When set, the list is authoritative: a named platform is on, an unnamed one is
# off, whatever the environment or a probe says. That is the point — a build
# declares its platforms once and stops depending on how it was invoked.
#
# Applied by setting a NORMAL variable per platform, which shadows the cache
# entry for the rest of this configure. Normal variables are recomputed every
# run, so editing the list takes effect immediately; writing the cache would
# leave an entry option() then refuses to update and a later run would silently
# honour.
if(SOLIDSYSLOG_PLATFORMS)
    foreach(requested_platform IN LISTS SOLIDSYSLOG_PLATFORMS)
        if(NOT requested_platform IN_LIST SOLIDSYSLOG_PLATFORM_TOKENS)
            message(FATAL_ERROR
                "SOLIDSYSLOG_PLATFORMS names an unknown platform '${requested_platform}'. "
                "Valid platforms: ${SOLIDSYSLOG_PLATFORM_TOKENS}.")
        endif()
    endforeach()

    foreach(row IN LISTS SOLIDSYSLOG_PLATFORM_REGISTRY)
        solidsyslog_read_platform("${row}")
        if(platform_token IN_LIST SOLIDSYSLOG_PLATFORMS)
            # Only a probe can say "no". An upstream platform's default is a
            # hint about the environment, never a capability — the sources
            # compile in the consumer's target against the consumer's config, so
            # naming one with a bare environment is exactly the supported case.
            if(platform_kind STREQUAL "probe" AND NOT platform_default)
                message(FATAL_ERROR
                    "SOLIDSYSLOG_PLATFORMS names '${platform_token}', which this toolchain "
                    "cannot provide — the availability probe for it failed. Drop it from the "
                    "list, or build where it is available.")
            endif()
            set(${platform_option} ON)
        else()
            set(${platform_option} OFF)
        endif()
    endforeach()
endif()

if(SOLIDSYSLOG_OPENSSL AND OPENSSL_VERSION VERSION_LESS 3.0)
    message(FATAL_ERROR
        "SOLIDSYSLOG_OPENSSL requires OpenSSL 3.0 or later; found ${OPENSSL_VERSION}. "
        "The integration harness uses EVP_RSA_gen (3.0+).")
endif()

# Winsock is not separately selectable: Platform/Windows is one bundle, and
# bundling the roles an OS fills is deliberate — it is what makes the host case
# a single name. Derived rather than cached so it tracks the platform list on
# every configure.
if(SOLIDSYSLOG_WINDOWS AND HAVE_WINSOCK)
    set(SOLIDSYSLOG_WINSOCK TRUE)
else()
    set(SOLIDSYSLOG_WINSOCK FALSE)
endif()

if(SOLIDSYSLOG_ATOMICS OR (SOLIDSYSLOG_WINDOWS AND HAVE_WINDOWS_INTERLOCKED))
    set(HAVE_ATOMIC_COUNTER TRUE)
else()
    set(HAVE_ATOMIC_COUNTER FALSE)
endif()

# What was selected and what it provides, however it was selected. The token
# line answers "did my -D take?"; the role line answers "is my stack complete?",
# which is the question a missing platform actually shows up as — a Store with
# no file, a CircularBuffer with no mutex.
# The vocabulary comes from the registry rather than a list restated here, so
# adding a role to a row shows up in the report instead of doing nothing.
set(solidsyslog_all_roles "")
foreach(row IN LISTS SOLIDSYSLOG_PLATFORM_REGISTRY)
    solidsyslog_read_platform("${row}")
    string(REPLACE " " ";" platform_role_list "${platform_roles}")
    list(APPEND solidsyslog_all_roles ${platform_role_list})
endforeach()
list(REMOVE_DUPLICATES solidsyslog_all_roles)

set(solidsyslog_selected_platforms "")
foreach(role IN LISTS solidsyslog_all_roles)
    set(solidsyslog_role_${role} "")
endforeach()

foreach(row IN LISTS SOLIDSYSLOG_PLATFORM_REGISTRY)
    solidsyslog_read_platform("${row}")
    if(${platform_option})
        list(APPEND solidsyslog_selected_platforms ${platform_token})
        string(REPLACE " " ";" platform_role_list "${platform_roles}")
        foreach(role IN LISTS platform_role_list)
            list(APPEND solidsyslog_role_${role} ${platform_token})
        endforeach()
    endif()
endforeach()

if(NOT solidsyslog_selected_platforms)
    set(solidsyslog_selected_platforms "(none)")
endif()
message(STATUS "SolidSyslog platforms: ${solidsyslog_selected_platforms}")

set(solidsyslog_role_report "")
foreach(role IN LISTS solidsyslog_all_roles)
    if(solidsyslog_role_${role})
        string(REPLACE ";" "," solidsyslog_role_names "${solidsyslog_role_${role}}")
    else()
        set(solidsyslog_role_names "(none)")
    endif()
    string(APPEND solidsyslog_role_report "  ${role}=${solidsyslog_role_names}")
endforeach()
message(STATUS "SolidSyslog roles:    ${solidsyslog_role_report}")

# Where each upstream tree lives. Selecting a pack does not need this — the pack
# carries its sources and the consumer's own target supplies the headers. What
# needs it is OUR unit tests and BDD targets, which compile against the real
# upstream. Cache variables with the same environment fallback, so every
# container and CI lane that exports these keeps working untouched, and anyone
# with a hand-placed checkout can pass -DSOLIDSYSLOG_LWIP_PATH=... instead of
# arranging their shell. A -D on the command line wins: set() without FORCE
# leaves an existing cache entry alone.
function(solidsyslog_declare_path variable_name help environment_fallback)
    set(${variable_name} "$ENV{${environment_fallback}}" CACHE PATH "${help}")
endfunction()

solidsyslog_declare_path(SOLIDSYSLOG_FREERTOS_KERNEL_PATH
    "Path to the FreeRTOS-Kernel source tree (SolidSyslog's own tests and BDD targets only)"
    FREERTOS_KERNEL_PATH)

solidsyslog_declare_path(SOLIDSYSLOG_FREERTOS_PLUS_TCP_PATH
    "Path to the FreeRTOS-Plus-TCP source tree (SolidSyslog's own tests and BDD targets only)"
    FREERTOS_PLUS_TCP_PATH)

solidsyslog_declare_path(SOLIDSYSLOG_FREERTOS_PLUS_FAT_PATH
    "Path to the FreeRTOS-Plus-FAT source tree (SolidSyslog's own tests and BDD targets only)"
    FREERTOS_PLUS_FAT_PATH)

solidsyslog_declare_path(SOLIDSYSLOG_LWIP_PATH
    "Path to the lwIP source tree (SolidSyslog's own tests and BDD targets only)"
    LWIP_PATH)

solidsyslog_declare_path(SOLIDSYSLOG_MBEDTLS_DIR
    "Path to the Mbed TLS source tree (SolidSyslog's own tests and BDD targets only)"
    MBEDTLS_DIR)

solidsyslog_declare_path(SOLIDSYSLOG_FATFS_PATH
    "Path to the ChaN FatFs source tree (SolidSyslog's own tests and BDD targets only)"
    FATFS_PATH)

# Selecting a pack without telling us where its upstream lives is perfectly
# valid for a consumer, and impossible for our own suite. Name the variable to
# set rather than failing three directories down on a missing lwip/opt.h.
function(solidsyslog_require_path_for_tests pack_option path_variable environment_fallback)
    if(SOLIDSYSLOG_BUILD_TESTING AND ${pack_option} AND NOT ${path_variable})
        message(FATAL_ERROR
            "${pack_option}=ON with SOLIDSYSLOG_BUILD_TESTING=ON, but ${path_variable} "
            "is empty. The pack itself needs no path — its sources compile against your "
            "config header — but SolidSyslog's own tests compile against the real "
            "upstream headers. Pass -D${path_variable}=/path/to/tree, export "
            "${environment_fallback}, or turn the pack off with -D${pack_option}=OFF.")
    endif()
endfunction()

solidsyslog_require_path_for_tests(SOLIDSYSLOG_FREERTOS
    SOLIDSYSLOG_FREERTOS_KERNEL_PATH FREERTOS_KERNEL_PATH)
solidsyslog_require_path_for_tests(SOLIDSYSLOG_LWIP
    SOLIDSYSLOG_LWIP_PATH LWIP_PATH)
solidsyslog_require_path_for_tests(SOLIDSYSLOG_MBEDTLS
    SOLIDSYSLOG_MBEDTLS_DIR MBEDTLS_DIR)
solidsyslog_require_path_for_tests(SOLIDSYSLOG_FATFS
    SOLIDSYSLOG_FATFS_PATH FATFS_PATH)
solidsyslog_require_path_for_tests(SOLIDSYSLOG_PLUSFAT
    SOLIDSYSLOG_FREERTOS_PLUS_FAT_PATH FREERTOS_PLUS_FAT_PATH)
# Plus-TCP is deliberately absent: Tests/FreeRtos skips only the tests that need
# it, so a kernel-only environment still builds the UART test.

# Allocation strategy (E11). Currently only "static" ships — internal static
# pools allocate instance bookkeeping for stateful classes. A future epic
# may add "dynamic" (heap-backed). Selecting "dynamic" now errors hard so
# downstream builds fail fast rather than silently linking against missing
# TUs.
set(SOLIDSYSLOG_ALLOCATION_STRATEGY "static" CACHE STRING
    "Allocation strategy for instance bookkeeping: static (internal pools) or dynamic (heap, not yet shipped)")
set_property(CACHE SOLIDSYSLOG_ALLOCATION_STRATEGY PROPERTY STRINGS static dynamic)
if(SOLIDSYSLOG_ALLOCATION_STRATEGY STREQUAL "dynamic")
    message(FATAL_ERROR
        "SOLIDSYSLOG_ALLOCATION_STRATEGY=dynamic is reserved for a future epic; "
        "no dynamic TUs ship yet. Set SOLIDSYSLOG_ALLOCATION_STRATEGY=static (the default).")
elseif(NOT SOLIDSYSLOG_ALLOCATION_STRATEGY STREQUAL "static")
    message(FATAL_ERROR
        "SOLIDSYSLOG_ALLOCATION_STRATEGY must be 'static' (got: '${SOLIDSYSLOG_ALLOCATION_STRATEGY}').")
endif()

# Which FreeRTOS BDD target the cross build produces (E28). This is maintainer
# tooling, not a consumer knob: BDD targets are top-level-only, and since S30.06
# it no longer selects any platform — PlusTcp and LwipRaw are named in
# SOLIDSYSLOG_PLATFORMS like every other platform. It survives only to answer
# "which ELF does a freertos-cross preset build?", which is why the name says so.
if(SOLIDSYSLOG_IS_TOP_LEVEL)
    set(SOLIDSYSLOG_BDD_FREERTOS_NET "PLUSTCP" CACHE STRING
        "Which FreeRTOS BDD target the cross build produces: PLUSTCP (default) or LWIP")
    set_property(CACHE SOLIDSYSLOG_BDD_FREERTOS_NET PROPERTY STRINGS PLUSTCP LWIP)
    if(NOT SOLIDSYSLOG_BDD_FREERTOS_NET STREQUAL "PLUSTCP"
       AND NOT SOLIDSYSLOG_BDD_FREERTOS_NET STREQUAL "LWIP")
        message(FATAL_ERROR
            "SOLIDSYSLOG_BDD_FREERTOS_NET must be PLUSTCP or LWIP "
            "(got: '${SOLIDSYSLOG_BDD_FREERTOS_NET}').")
    endif()
endif()

# Build-time tunables (E21: Port-Time Configurability). Integrators override
# library defaults by setting SOLIDSYSLOG_USER_TUNABLES_FILE to the absolute
# path of their own header containing #define lines for whichever tunables
# they want to change. Defaults live in Core/Interface/SolidSyslogTunablesDefaults.h.
add_library(SolidSyslogTunables INTERFACE)
target_include_directories(SolidSyslogTunables INTERFACE
    ${SolidSyslog_SOURCE_DIR}/Core/Interface)
if(SOLIDSYSLOG_USER_TUNABLES_FILE)
    target_compile_definitions(SolidSyslogTunables INTERFACE
        SOLIDSYSLOG_USER_TUNABLES_FILE="${SOLIDSYSLOG_USER_TUNABLES_FILE}")
endif()

# BDD-side mirror of the configured tunables — eliminates the hardcoded
# "remember to bump this" mirror that used to live in syslog_steps.py.
# Parse the default from SolidSyslogTunablesDefaults.h; if a user override
# is in effect and defines the macro, use its value instead. The generated
# file is written to the source tree because behave's import discovery
# expects modules under Bdd/features/steps/; it is gitignored. Top-level only:
# configuring a dependency must never write into that dependency's checkout.
if(SOLIDSYSLOG_IS_TOP_LEVEL)
    file(READ "${SolidSyslog_SOURCE_DIR}/Core/Interface/SolidSyslogTunablesDefaults.h" _solidsyslog_defaults)
    string(REGEX MATCH "#define[ \t]+SOLIDSYSLOG_MAX_MESSAGE_SIZE[ \t]+([0-9]+)" _ "${_solidsyslog_defaults}")
    set(SOLIDSYSLOG_BDD_MAX_MESSAGE_SIZE "${CMAKE_MATCH_1}")
    if(SOLIDSYSLOG_USER_TUNABLES_FILE AND EXISTS "${SOLIDSYSLOG_USER_TUNABLES_FILE}")
        file(READ "${SOLIDSYSLOG_USER_TUNABLES_FILE}" _solidsyslog_user_tunables)
        string(REGEX MATCH "#define[ \t]+SOLIDSYSLOG_MAX_MESSAGE_SIZE[ \t]+([0-9]+)" _ "${_solidsyslog_user_tunables}")
        if(CMAKE_MATCH_1)
            set(SOLIDSYSLOG_BDD_MAX_MESSAGE_SIZE "${CMAKE_MATCH_1}")
        endif()
    endif()
    configure_file(
        "${SolidSyslog_SOURCE_DIR}/Bdd/features/steps/solidsyslog_tunables.py.in"
        "${SolidSyslog_SOURCE_DIR}/Bdd/features/steps/solidsyslog_tunables.py"
        @ONLY
    )
endif()

add_subdirectory(Core/Source)

# Every selected platform, in registry order. Platforms are independent — no
# Platform/ directory references another — so the order is presentational.
# Core/Source has already run, which matters for the precompiled ones: they
# reach back with target_sources(SolidSyslog PRIVATE ...).
foreach(row IN LISTS SOLIDSYSLOG_PLATFORM_REGISTRY)
    solidsyslog_read_platform("${row}")
    if(${platform_option})
        add_subdirectory(${platform_directory})
    endif()
endforeach()

# Host BDD targets — top-level only, and only where a host network stack exists
# to drive them.
if(SOLIDSYSLOG_IS_TOP_LEVEL
   AND (SOLIDSYSLOG_POSIX OR (SOLIDSYSLOG_WINSOCK AND HAVE_WINDOWS_PLATFORM)))
    add_subdirectory(Bdd/Targets)
endif()

# The cross-compiled FreeRTOS target is selected by the arm-none-eabi
# toolchain file (see Bdd/Targets/FreeRtos/cmake/arm-none-eabi.cmake); it
# only enters the build when a freertos-cross preset is active. The chosen
# BDD backend selector picks which target: PLUSTCP builds the full
# FreeRTOS-Plus-TCP BDD target; LWIP builds the FreeRtosLwip BDD target
# (S28.09) — LAN9118 netif + NO_SYS=0 lwIP + LwipRaw adapters, no PlusTcp.
#
# The top-level gate matters most here: an integrator cross-compiling for
# Cortex-M sets exactly these two variables in their own toolchain file, and
# without it their build would try to produce SolidSyslog's BDD ELF.
if(SOLIDSYSLOG_IS_TOP_LEVEL
   AND CMAKE_CROSSCOMPILING
   AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
    if(SOLIDSYSLOG_BDD_FREERTOS_NET STREQUAL "LWIP")
        add_subdirectory(Bdd/Targets/FreeRtosLwip)
    else()
        add_subdirectory(Bdd/Targets/FreeRtos)
    endif()
endif()

if(SOLIDSYSLOG_BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_subdirectory(Tests)
endif()

# Integration manifest (S30.03) — generated from the configured targets (Core
# SOURCES + each selected pack's INTERFACE_SOURCES) so the non-CMake
# file/include/-D list can't drift from the build. Runs last, after every pack
# target is defined. `cmake --build <dir> --target manifest` prints it. See
# cmake/Manifest.cmake.
if(SOLIDSYSLOG_IS_TOP_LEVEL)
    solidsyslog_generate_manifest()
endif()
