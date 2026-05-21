cmake_minimum_required(VERSION 3.16)
project(SolidSyslog C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(LayerGuard)
solidsyslog_enforce_layering()
include(Iwyu)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_CROSSCOMPILING)
    find_package(CppUTest REQUIRED)
    find_package(Threads REQUIRED)
endif()

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
            "--inline-suppr"
            "--std=c11"
        )
        set(CMAKE_CXX_CPPCHECK
            "${CPPCHECK_EXECUTABLE}"
            "--enable=warning,style,performance,portability"
            "--error-exitcode=1"
            "--suppress=missingIncludeSystem"
            "--suppress=useStlAlgorithm"
            "--inline-suppr"
            "--std=c++17"
        )
    else()
        message(WARNING "cppcheck not found — ENABLE_CPPCHECK has no effect")
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

include(CheckIncludeFile)
check_include_file(stdatomic.h HAVE_STDATOMIC_H)
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

# mbedTLS-based TLS is the embedded-target reference TLS backend (OpenSSL is too
# large for Cortex-M). Default ON when the MBEDTLS_DIR environment variable is
# set (the cpputest-freertos devcontainer image provides /opt/mbedtls); pass
# -DSOLIDSYSLOG_MBEDTLS=OFF to disable explicitly. Unlike OpenSSL there is no
# find_package — mbedTLS is header-configured (mbedtls_config.h changes ABI),
# so the adapter ships as an INTERFACE pack and consumers compile the sources
# themselves with their own config. See Platform/MbedTls/CMakeLists.txt.
if(DEFINED ENV{MBEDTLS_DIR})
    set(_solidsyslog_mbedtls_default ON)
else()
    set(_solidsyslog_mbedtls_default OFF)
endif()
option(SOLIDSYSLOG_MBEDTLS "Include mbedTLS-based TLS support" ${_solidsyslog_mbedtls_default})
unset(_solidsyslog_mbedtls_default)
if(SOLIDSYSLOG_MBEDTLS AND NOT DEFINED ENV{MBEDTLS_DIR})
    message(FATAL_ERROR
        "SOLIDSYSLOG_MBEDTLS=ON but MBEDTLS_DIR is not set; "
        "export MBEDTLS_DIR=/path/to/mbedtls or pass -DSOLIDSYSLOG_MBEDTLS=OFF.")
endif()

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

# Build-time tunables (E21: Port-Time Configurability). Integrators override
# library defaults by setting SOLIDSYSLOG_USER_TUNABLES_FILE to the absolute
# path of their own header containing #define lines for whichever tunables
# they want to change. Defaults live in Core/Interface/SolidSyslogTunablesDefaults.h.
add_library(SolidSyslogTunables INTERFACE)
target_include_directories(SolidSyslogTunables INTERFACE
    ${CMAKE_SOURCE_DIR}/Core/Interface)
if(SOLIDSYSLOG_USER_TUNABLES_FILE)
    target_compile_definitions(SolidSyslogTunables INTERFACE
        SOLIDSYSLOG_USER_TUNABLES_FILE="${SOLIDSYSLOG_USER_TUNABLES_FILE}")
endif()

# BDD-side mirror of the configured tunables — eliminates the hardcoded
# "remember to bump this" mirror that used to live in syslog_steps.py.
# Parse the default from SolidSyslogTunablesDefaults.h; if a user override
# is in effect and defines the macro, use its value instead. The generated
# file is written to the source tree because behave's import discovery
# expects modules under Bdd/features/steps/; it is gitignored.
file(READ "${CMAKE_SOURCE_DIR}/Core/Interface/SolidSyslogTunablesDefaults.h" _solidsyslog_defaults)
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
    "${CMAKE_SOURCE_DIR}/Bdd/features/steps/solidsyslog_tunables.py.in"
    "${CMAKE_SOURCE_DIR}/Bdd/features/steps/solidsyslog_tunables.py"
    @ONLY
)

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

if(SOLIDSYSLOG_POSIX OR (SOLIDSYSLOG_WINSOCK AND HAVE_WINDOWS_PLATFORM))
    add_subdirectory(Bdd/Targets)
endif()

# FreeRTOS adapters ship as sources via an INTERFACE library — the integrator's
# FreeRTOSConfig.h flows into each consumer's compile (see
# project_header_configured_platforms memory). Gated on the upstream sources
# being present in the build environment.
if(DEFINED ENV{FREERTOS_KERNEL_PATH})
    add_subdirectory(Platform/FreeRtos)
endif()

# FatFs adapters likewise ship as sources via an INTERFACE library — the
# integrator's ffconf.h flows into each consumer's compile. Gated on the
# upstream FatFs sources being present in the build environment.
if(DEFINED ENV{FATFS_PATH})
    add_subdirectory(Platform/FatFs)
endif()

# The cross-compiled FreeRTOS BDD target is selected by the arm-none-eabi
# toolchain file (see Bdd/Targets/FreeRtos/cmake/arm-none-eabi.cmake); it
# only enters the build when a freertos-cross preset is active.
if(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
    add_subdirectory(Bdd/Targets/FreeRtos)
endif()

if(NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_subdirectory(Tests)
endif()
