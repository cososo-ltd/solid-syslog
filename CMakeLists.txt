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

include(CheckSymbolExists)
check_symbol_exists(sendto "sys/socket.h" HAVE_POSIX_SOCKETS)
option(SOLIDSYSLOG_POSIX "Include POSIX UDP sender" ${HAVE_POSIX_SOCKETS})

set(_SOLIDSYSLOG_PREV_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
set(CMAKE_REQUIRED_LIBRARIES ws2_32)
check_symbol_exists(sendto "winsock2.h" HAVE_WINSOCK)
set(CMAKE_REQUIRED_LIBRARIES "${_SOLIDSYSLOG_PREV_CMAKE_REQUIRED_LIBRARIES}")
unset(_SOLIDSYSLOG_PREV_CMAKE_REQUIRED_LIBRARIES)
option(SOLIDSYSLOG_WINSOCK "Include Winsock UDP sender" ${HAVE_WINSOCK})

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

if(HAVE_STDATOMIC_H OR HAVE_WINDOWS_INTERLOCKED)
    set(HAVE_ATOMIC_COUNTER TRUE)
endif()

# OpenSSL-based TLS is an optional platform feature. Default ON when OpenSSL is
# discoverable so CI and most workstations get TLS automatically; pass
# -DSOLIDSYSLOG_OPENSSL=OFF to build a UDP/TCP-only library on systems without
# OpenSSL.
find_package(OpenSSL QUIET)
option(SOLIDSYSLOG_OPENSSL "Include OpenSSL-based TLS support" ${OpenSSL_FOUND})
if(SOLIDSYSLOG_OPENSSL)
    if(NOT OpenSSL_FOUND)
        message(FATAL_ERROR "SOLIDSYSLOG_OPENSSL=ON but OpenSSL was not found; install OpenSSL or pass -DSOLIDSYSLOG_OPENSSL=OFF")
    endif()
    if(OPENSSL_VERSION VERSION_LESS 3.0)
        message(FATAL_ERROR "SOLIDSYSLOG_OPENSSL requires OpenSSL 3.0 or later; found ${OPENSSL_VERSION}. The integration harness uses EVP_RSA_gen (3.0+).")
    endif()
endif()

# Header-configured platform packs (S30.05).
#
# Unlike the host roles above, these have no find_package and no compile probe:
# each is header-configured (lwipopts.h, FreeRTOSConfig.h, mbedtls_config.h and
# ffconf.h all change layout and behaviour), so the pack ships as an INTERFACE
# library carrying its own adapter sources and the consumer compiles them
# against their own config. A pack therefore never needs to know where its
# upstream tree lives — the consumer's target is what puts lwip/*.h, FreeRTOS.h,
# mbedtls/*.h and ff.h on the include path.
#
# Selection is one switch per pack, defaulting to "the matching upstream path is
# on the environment". That default is what every preset, devcontainer service
# and CI lane here already expresses, so all of them configure exactly as
# before. What it adds is a way to say so explicitly: an integrator who vendors
# lwIP inside their own tree and exports nothing passes -DSOLIDSYSLOG_LWIP=ON.
# Before S30.05 the environment was the only vocabulary, so that integrator got
# a silently absent pack and a generate-time "target not found" naming
# SolidSyslog::LwipRaw with nothing pointing at the cause.
#
# The upstream paths have not gone away — our own unit tests and BDD targets
# compile against the real upstream headers and still need them. They are
# separate variables, demanded only where they are genuinely used.
function(solidsyslog_declare_pack option_name help environment_fallback)
    if(DEFINED ENV{${environment_fallback}})
        set(selected_by_default ON)
    else()
        set(selected_by_default OFF)
    endif()
    option(${option_name} "${help}" ${selected_by_default})
endfunction()

solidsyslog_declare_pack(SOLIDSYSLOG_FREERTOS
    "Include the FreeRTOS platform pack (Mutex, SysUpTime, and the Plus-TCP networking adapters)"
    FREERTOS_KERNEL_PATH)

solidsyslog_declare_pack(SOLIDSYSLOG_LWIP
    "Include the lwIP Raw API platform pack (Address, Resolver, Datagram, TcpStream)"
    LWIP_PATH)

# The embedded-target reference TLS backend — OpenSSL is too large for Cortex-M.
solidsyslog_declare_pack(SOLIDSYSLOG_MBEDTLS
    "Include mbedTLS-based TLS support"
    MBEDTLS_DIR)

solidsyslog_declare_pack(SOLIDSYSLOG_FATFS
    "Include the ChaN FatFs platform pack (File)"
    FATFS_PATH)

solidsyslog_declare_pack(SOLIDSYSLOG_PLUSFAT
    "Include the FreeRTOS-Plus-FAT platform pack (File)"
    FREERTOS_PLUS_FAT_PATH)

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

# FreeRTOS networking backend (E28). Two adapter trees ship in the same
# library: FreeRTOS-Plus-TCP (current) and lwIP Raw (new). The integrator
# picks one — or both, for dev-container context-switching — at configure
# time. CI builds PLUSTCP and LWIP in isolation; BOTH is a dev-container
# convenience.
#
# S28.03-S28.05 land the OS-agnostic slices (Address, Resolver, Datagram,
# TcpStream). LWIP and BOTH configure but the backend is still partial —
# BDD arrives in S28.06. Platform/LwipRaw is OS-agnostic by construction
# (wraps lwIP only, no FreeRTOS coupling) and is gated separately on
# LWIP_PATH below; this option only controls which adapter pack the
# FreeRTOS BDD target wires up.
set(SOLIDSYSLOG_FREERTOS_NET "PLUSTCP" CACHE STRING
    "FreeRTOS networking backend: PLUSTCP (default), LWIP, or BOTH")
set_property(CACHE SOLIDSYSLOG_FREERTOS_NET PROPERTY STRINGS PLUSTCP LWIP BOTH)
if(NOT SOLIDSYSLOG_FREERTOS_NET STREQUAL "PLUSTCP"
   AND NOT SOLIDSYSLOG_FREERTOS_NET STREQUAL "LWIP"
   AND NOT SOLIDSYSLOG_FREERTOS_NET STREQUAL "BOTH")
    message(FATAL_ERROR
        "SOLIDSYSLOG_FREERTOS_NET must be one of PLUSTCP, LWIP, or BOTH "
        "(got: '${SOLIDSYSLOG_FREERTOS_NET}').")
endif()
if(SOLIDSYSLOG_FREERTOS_NET STREQUAL "LWIP")
    message(STATUS
        "SOLIDSYSLOG_FREERTOS_NET=LWIP: cross builds the FreeRtosLwip BDD "
        "target (LAN9118 netif + NO_SYS=0 tcpip + LwipRaw adapters), run on "
        "QEMU mps2-an385 by the advisory bdd-freertos-qemu-lwip lane (S28.09).")
endif()
if(SOLIDSYSLOG_FREERTOS_NET STREQUAL "BOTH")
    message(STATUS
        "SOLIDSYSLOG_FREERTOS_NET=BOTH: cross builds the PlusTcp BDD target; "
        "the lwIP backend cross-builds in isolation under NET=LWIP.")
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

if(HAVE_STDATOMIC_H)
    add_subdirectory(Platform/Atomics)
endif()

if(SOLIDSYSLOG_POSIX)
    add_subdirectory(Platform/Posix)
endif()

if(SOLIDSYSLOG_OPENSSL)
    add_subdirectory(Platform/OpenSsl)
endif()

if(SOLIDSYSLOG_MBEDTLS)
    add_subdirectory(Platform/MbedTls)
endif()

if(HAVE_WINDOWS_INTERLOCKED OR HAVE_WINDOWS_PLATFORM OR SOLIDSYSLOG_WINSOCK)
    add_subdirectory(Platform/Windows)
endif()

if(SOLIDSYSLOG_IS_TOP_LEVEL
   AND (SOLIDSYSLOG_POSIX OR (SOLIDSYSLOG_WINSOCK AND HAVE_WINDOWS_PLATFORM)))
    add_subdirectory(Bdd/Targets)
endif()

# FreeRTOS adapters ship as sources via INTERFACE libraries — the integrator's
# FreeRTOSConfig.h / FreeRTOSIPConfig.h flows into each consumer's compile (see
# project_header_configured_platforms memory). Platform/FreeRtos hosts the
# RTOS primitives (Mutex, SysUpTime) — always added with the pack, regardless of
# the chosen networking backend. Platform/PlusTcp hosts the FreeRTOS-Plus-TCP
# networking adapters (Address/Datagram/Resolver/TcpStream) — added when
# SOLIDSYSLOG_FREERTOS_NET selects PLUSTCP or BOTH.
if(SOLIDSYSLOG_FREERTOS)
    add_subdirectory(Platform/FreeRtos)
    if(SOLIDSYSLOG_FREERTOS_NET STREQUAL "PLUSTCP"
       OR SOLIDSYSLOG_FREERTOS_NET STREQUAL "BOTH")
        add_subdirectory(Platform/PlusTcp)
    endif()
endif()

# Platform/LwipRaw — OS-agnostic wrapper of lwIP Raw API. Selected independently
# of SOLIDSYSLOG_FREERTOS (lwIP runs on bare metal, Zephyr, ThreadX, FreeRTOS,
# ...). SOLIDSYSLOG_FREERTOS_NET only governs which adapter pack the FreeRTOS BDD
# target links against — it does not gate this library's existence.
if(SOLIDSYSLOG_LWIP)
    add_subdirectory(Platform/LwipRaw)
endif()

# FatFs adapters likewise ship as sources via an INTERFACE library — the
# integrator's ffconf.h flows into each consumer's compile.
if(SOLIDSYSLOG_FATFS)
    add_subdirectory(Platform/FatFs)
endif()

# FreeRTOS-Plus-FAT adapters ship as sources via an INTERFACE library too — the
# integrator's FreeRTOSFATConfig.h flows into each consumer's compile.
if(SOLIDSYSLOG_PLUSFAT)
    add_subdirectory(Platform/PlusFat)
endif()

# The cross-compiled FreeRTOS target is selected by the arm-none-eabi
# toolchain file (see Bdd/Targets/FreeRtos/cmake/arm-none-eabi.cmake); it
# only enters the build when a freertos-cross preset is active. The chosen
# networking backend picks which target: PLUSTCP/BOTH builds the full
# FreeRTOS-Plus-TCP BDD target; LWIP builds the FreeRtosLwip BDD target
# (S28.09) — LAN9118 netif + NO_SYS=0 lwIP + LwipRaw adapters, no PlusTcp.
#
# The top-level gate matters most here: an integrator cross-compiling for
# Cortex-M sets exactly these two variables in their own toolchain file, and
# without it their build would try to produce SolidSyslog's BDD ELF.
if(SOLIDSYSLOG_IS_TOP_LEVEL
   AND CMAKE_CROSSCOMPILING
   AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
    if(SOLIDSYSLOG_FREERTOS_NET STREQUAL "LWIP")
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
