# CMSIS-RTOS2 + lwIP BDD target ELF for QEMU mps2-an385 (Cortex-M3).
#
# The fourth BDD target (S40.01 #850). It is the home for the three packs E40,
# E36 and E35 add: each is swapped in against a lane that is already green, so a
# failure is attributable to the one thing that changed.
#
# Its own code speaks CMSIS-RTOS2, not the kernel underneath: the OS pack is
# CmsisRtos, and everything kernel-specific - the kernel sources, its config
# header, its application hooks and the RTOS2 wrapper - sits in
# Kernel/FreeRtos/. A second kernel is a sibling directory there and a one-line
# change to the include below. What still calls the kernel directly is the lwIP
# port layer and the Ethernet driver, which sit beneath the wrapper.
#
# Source layout mirrors Bdd/Targets/FreeRtosLwip/CMakeLists.txt: upstream
# FreeRTOS-Kernel + lwIP-core + lwIP-api + the contrib FreeRTOS sys_arch + the
# vendored LAN9118 driver compile into a separate OBJECT library
# (`solid_syslog_cmsis_lwip_upstream`) under a relaxed warning set; our own code
# (main.c, the reused Cortex-M3 startup, the shared LAN9118 netif wrapper, the
# platform adapters and the shared BddTarget sources) stays in the executable
# under the strict project warning bar.

# What belongs to the board - see Bdd/Targets/Boards/Board.cmake. First, because
# the kernel half below takes its port from it.
include(${CMAKE_SOURCE_DIR}/Bdd/Targets/Boards/Board.cmake)

# The kernel half: sources, include dirs and application hooks. Swapping
# kernels is changing this one path.
include(${CMAKE_CURRENT_SOURCE_DIR}/Kernel/FreeRtos/Kernel.cmake)

# Shared with the sibling lwIP target - see Bdd/Targets/Common/lwip/.
set(LWIP_SHARED_DIR "${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/lwip")

# The upstream trees this target wires, beyond the kernel's - see
# Bdd/Targets/Common/UpstreamTrees.cmake.
solidsyslog_bdd_upstream(LWIP_DIR SOLIDSYSLOG_LWIP_PATH /opt/lwip)
solidsyslog_bdd_upstream(LITTLEFS_PATH SOLIDSYSLOG_LITTLEFS_PATH /opt/littlefs)

# lwIP ships its source lists as *_SRCS variables via Filelists.cmake. For
# NO_SYS=0 we consume the IPv4 core set plus the api/ set, which already names
# every translation unit the sockets layer needs — sockets.c, api_lib.c,
# api_msg.c, netbuf.c and netdb.c alongside tcpip.c. They are compiled either
# way; LWIP_SOCKET in lwipopts.h is what makes them more than empty, at a cost
# of about 12 KB of text. No ipv6.
include(${LWIP_DIR}/src/Filelists.cmake)

set(LWIP_CONTRIB_FREERTOS_DIR "${LWIP_DIR}/contrib/ports/freertos")


# --- Upstream OBJECT library --------------------------------------------------
#
# FreeRTOS kernel + lwIP IPv4 core + lwIP api (tcpip thread) + the contrib
# FreeRTOS sys_arch + the vendored Arm LAN9118 driver, compiled with the
# relaxed warning set those upstream / third-party sources require. The
# executable consumes the objects via $<TARGET_OBJECTS:...> so the relaxations
# don't leak into project code.

add_library(solid_syslog_cmsis_lwip_upstream OBJECT
    ${BDD_KERNEL_UPSTREAM_SOURCES}
    ${lwipcore_SRCS}
    ${lwipcore4_SRCS}
    ${lwipapi_SRCS}
    # etharp (in core4) emits ARP frames via ethernet_output/ethbroadcast/
    # ethzero from netif/ethernet.c — pull that one netif source in (but not
    # bridgeif/slipif from lwipnetif_SRCS, which this target has no use for).
    ${LWIP_DIR}/src/netif/ethernet.c
    # lwIP FreeRTOS OS-abstraction port (sys_mutex/sem/mbox/thread) — required
    # for NO_SYS=0.
    ${LWIP_CONTRIB_FREERTOS_DIR}/sys_arch.c
    # The board's third-party Ethernet controller driver. Our netif over it
    # stays in the executable under the strict bar; this sits under the
    # relaxations.
    ${BDD_BOARD_LWIP_UPSTREAM_SOURCES}
    # LittleFS upstream, under the same relaxations: lfs.c raises well over a
    # hundred diagnostics against this project's -Werror set, which is not
    # upstream's problem and not ours to pin a version over.
    ${LITTLEFS_PATH}/lfs.c
    ${LITTLEFS_PATH}/lfs_util.c
)

target_compile_options(solid_syslog_cmsis_lwip_upstream PRIVATE
    ${BDD_BOARD_ARCH_FLAGS}
    -ffunction-sections
    -fdata-sections
    -fno-common
    # FreeRTOS-Kernel + lwIP + the Arm driver use non-prototype declarations,
    # type-narrowing constructs, sign-conversion patterns, and shadowed locals
    # that trip our strict host warnings. Scoped to this OBJECT library so the
    # relaxations never reach project code.
    -Wno-unused-parameter
    -Wno-unused-but-set-variable
    -Wno-implicit-fallthrough
    -Wno-array-bounds
    -Wno-conversion
    -Wno-sign-conversion
    -Wno-shadow
    -Wno-pedantic
)

target_include_directories(solid_syslog_cmsis_lwip_upstream PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}                       # lwipopts.h
    ${BDD_KERNEL_UPSTREAM_INCLUDES}                   # FreeRTOSConfig.h, kernel, CMSIS-RTOS2
    ${BDD_BOARD_INCLUDES}                             # the device facts FreeRTOSConfig.h reads
    ${LWIP_SHARED_DIR}                                 # arch/cc.h
    ${BDD_BOARD_LWIP_INCLUDES}                        # the controller driver's headers
    ${LWIP_DIR}/src/include
    ${LWIP_CONTRIB_FREERTOS_DIR}/include              # arch/sys_arch.h
    ${LITTLEFS_PATH}                                  # lfs.h, lfs_util.h
)

# --- The TLS pack ----------------------------------------------------------
#
# One target, two builds: Mbed TLS by default, wolfSSL where the configure
# names the WolfSsl pack (the cmsis-cross-lwip-wolfssl preset). Each branch
# says which upstream it compiles, which of our sources fill the TLS and
# at-rest seams, and what the executable links and defines.
if(SOLIDSYSLOG_WOLFSSL)
    solidsyslog_bdd_upstream(WOLFSSL_SOURCE_DIR SOLIDSYSLOG_WOLFSSL_SOURCE_DIR /opt/wolfssl-src PROBE wolfssl/ssl.h)

    # wolfSSL from source, the way an embedded integrator builds it: the file
    # list, compiled under this target's user_settings.h. Its own CMake is not
    # used - it probes for a hosted system this target is not. Every file
    # gates itself on the settings, so one the build has no use for compiles
    # to nothing.
    set(WOLFSSL_USER_SETTINGS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/wolfssl")
    add_library(solid_syslog_cmsis_wolfssl_upstream OBJECT
        ${WOLFSSL_SOURCE_DIR}/src/internal.c
        ${WOLFSSL_SOURCE_DIR}/src/keys.c
        ${WOLFSSL_SOURCE_DIR}/src/ssl.c
        ${WOLFSSL_SOURCE_DIR}/src/tls.c
        ${WOLFSSL_SOURCE_DIR}/src/tls13.c
        ${WOLFSSL_SOURCE_DIR}/src/wolfio.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/aes.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/asn.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/coding.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/dh.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/ecc.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/error.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/hash.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/hmac.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/kdf.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/logging.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/memory.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/random.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/rsa.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/sha.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/sha256.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/sha512.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/signature.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/sp_int.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/wc_encrypt.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/wc_port.c
        ${WOLFSSL_SOURCE_DIR}/wolfcrypt/src/wolfmath.c
    )
    target_compile_options(solid_syslog_cmsis_wolfssl_upstream PRIVATE
        ${BDD_BOARD_ARCH_FLAGS}
        -ffunction-sections
        -fdata-sections
        -fno-common
        -w
    )
    target_compile_definitions(solid_syslog_cmsis_wolfssl_upstream PRIVATE WOLFSSL_USER_SETTINGS)
    target_include_directories(solid_syslog_cmsis_wolfssl_upstream PRIVATE
        ${WOLFSSL_USER_SETTINGS_DIR}                  # user_settings.h
        ${WOLFSSL_SOURCE_DIR}
        ${BDD_KERNEL_UPSTREAM_INCLUDES}               # cmsis_os2.h, for wolfSSL's CMSIS-RTOS2 port
    )
    set_target_properties(solid_syslog_cmsis_wolfssl_upstream PROPERTIES
        C_CLANG_TIDY ""
        C_CPPCHECK ""
    )

    set(BDD_TLS_SOURCES
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetSecurityPolicy_WolfSsl.c
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetTlsSender_WolfSsl_InnerStream.c
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetWolfSslBakedCredentials.c
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetWolfSslPort.c
        $<TARGET_OBJECTS:solid_syslog_cmsis_wolfssl_upstream>
    )
    set(BDD_TLS_LIBRARIES
        SolidSyslog::WolfSsl                          # TLS Stream + HMAC/AES-GCM policy adapters (sources)
    )
    # The pack's sources, and ours that include wolfSSL headers, must see the
    # settings the library was compiled under. The handshake runs on the service
    # thread and peaks near 22 KiB of stack under these settings, read from the
    # [stack-hwm] line the target prints on `quit`, so this build gives that
    # thread more than the pipeline's default.
    set(BDD_TLS_DEFINITIONS WOLFSSL_USER_SETTINGS BDD_TARGET_SERVICE_STACK_BYTES=32768U)
    set(BDD_TLS_INCLUDES ${WOLFSSL_USER_SETTINGS_DIR})
    # wolfSSL's headers are not clean under this project's warning set, which
    # upstream does not build with. SYSTEM silences them without relaxing
    # anything we compile.
    set(BDD_TLS_SYSTEM_INCLUDES ${WOLFSSL_SOURCE_DIR})
else()
    solidsyslog_bdd_upstream(MBEDTLS_SOURCE_DIR SOLIDSYSLOG_MBEDTLS_DIR /opt/mbedtls PROBE CMakeLists.txt)

    # --- mbedTLS upstream as a subproject ----------------------------------------
    #
    # Same pattern as Bdd/Targets/FreeRtos/CMakeLists.txt: bring mbedTLS in via
    # add_subdirectory(EXCLUDE_FROM_ALL), force-disable its programs/tests, and
    # strip our strict warning surface from the upstream targets so a future
    # mbedTLS bump doesn't break our build for diagnostic reasons. clang-tidy /
    # cppcheck are also disabled per-target — they would otherwise apply our
    # .clang-tidy ruleset to upstream code.
    set(ENABLE_PROGRAMS OFF CACHE BOOL "Disable mbedTLS demo programs in our build" FORCE)
    set(ENABLE_TESTING OFF CACHE BOOL "Disable mbedTLS upstream tests in our build" FORCE)
    set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "Upstream mbedTLS warnings are not errors in our build" FORCE)
    add_subdirectory(${MBEDTLS_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/_mbedtls EXCLUDE_FROM_ALL)

    # MBEDTLS_USER_CONFIG_FILE is consumed by mbedTLS's headers via `#include`,
    # so the macro value must be a quoted C string literal. CMake-quote with
    # `\"` to survive the shell -> compiler hop.
    set(MBEDTLS_USER_CONFIG_HEADER
        "\"${CMAKE_CURRENT_SOURCE_DIR}/mbedtls_user_config.h\""
    )

    foreach(_mbedtls_upstream_target IN ITEMS mbedtls mbedx509 mbedcrypto everest p256m)
        if(TARGET ${_mbedtls_upstream_target})
            target_compile_options(${_mbedtls_upstream_target} PRIVATE -w)
            target_compile_definitions(${_mbedtls_upstream_target} PRIVATE
                MBEDTLS_USER_CONFIG_FILE=${MBEDTLS_USER_CONFIG_HEADER}
            )
            set_target_properties(${_mbedtls_upstream_target} PROPERTIES
                C_CLANG_TIDY ""
                CXX_CLANG_TIDY ""
                C_CPPCHECK ""
                CXX_CPPCHECK ""
            )
        endif()
    endforeach()

    set(BDD_TLS_SOURCES
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetSecurityPolicy_MbedTls.c
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetMbedTlsCredentials.c
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetTlsSender_MbedTls.c
    )
    set(BDD_TLS_LIBRARIES
        SolidSyslog::MbedTls                          # TLS Stream + HMAC/AES-GCM policy adapters (sources)
        mbedtls mbedx509 mbedcrypto                   # mbedTLS upstream (subproject, cross-compiled)
    )
    # Platform/MbedTls/Source/*.c include <mbedtls/ssl.h>; they MUST see the
    # same user config as the mbedTLS library itself, otherwise struct sizes
    # and the enabled-feature bitset diverge between the consumer and the
    # library.
    set(BDD_TLS_DEFINITIONS MBEDTLS_USER_CONFIG_FILE=${MBEDTLS_USER_CONFIG_HEADER})
    set(BDD_TLS_INCLUDES "")
    set(BDD_TLS_SYSTEM_INCLUDES "")
endif()

# --- Bake demo PEMs into the ELF -------------------------------------------
#
# QEMU has no path to the host filesystem, and the integrator config disables
# MBEDTLS_FS_IO, so the demo CA + client identity travel as `static const`
# arrays in rodata. xxd -i emits the array shape we want; mbedTLS's PEM parser
# additionally requires a NUL-terminated input, which the wrapper TU adds at
# parse time by copying into a +1-sized buffer (cheaper than a shell pipeline
# to append the NUL at bake time, and keeps this CMake step simple).

set(BAKED_PEMS_DIR "${CMAKE_CURRENT_BINARY_DIR}/baked_pems")
file(MAKE_DIRECTORY ${BAKED_PEMS_DIR})

function(bake_pem_into_header input_pem output_header symbol)
    add_custom_command(
        OUTPUT ${output_header}
        COMMAND bash -c "xxd -i -n '${symbol}' '${input_pem}' > '${output_header}'"
        DEPENDS ${input_pem}
        COMMENT "Baking ${input_pem} -> ${output_header}"
        VERBATIM
    )
endfunction()

bake_pem_into_header(
    ${CMAKE_SOURCE_DIR}/Bdd/syslog-ng/tls/ca.pem
    ${BAKED_PEMS_DIR}/BddBakedCaPem.h
    bdd_baked_ca_pem
)
# The authority a scenario rotates to, and the one that signs the certificates a
# peer-refusal cell expects to be refused. Baked like the rest: this target has
# no filesystem to read one from.
bake_pem_into_header(
    ${CMAKE_SOURCE_DIR}/Bdd/syslog-ng/tls/ca-b.pem
    ${BAKED_PEMS_DIR}/BddBakedCaBPem.h
    bdd_baked_ca_b_pem
)
bake_pem_into_header(
    ${CMAKE_SOURCE_DIR}/Bdd/syslog-ng/tls/client.pem
    ${BAKED_PEMS_DIR}/BddBakedClientCertPem.h
    bdd_baked_client_cert_pem
)
bake_pem_into_header(
    ${CMAKE_SOURCE_DIR}/Bdd/syslog-ng/tls/client.key
    ${BAKED_PEMS_DIR}/BddBakedClientKeyPem.h
    bdd_baked_client_key_pem
)

set(BAKED_PEM_HEADERS
    ${BAKED_PEMS_DIR}/BddBakedCaPem.h
    ${BAKED_PEMS_DIR}/BddBakedCaBPem.h
    ${BAKED_PEMS_DIR}/BddBakedClientCertPem.h
    ${BAKED_PEMS_DIR}/BddBakedClientKeyPem.h
)

# --- Executable ---------------------------------------------------------------
#
# Project code only. Inherits the full project warning set from the top-level
# CMakeLists.txt — no -Wno-* relaxations here.

add_executable(SolidSyslogBddTargetCmsis
    main.c
    ${BDD_BOARD_PROJECT_SOURCES}
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/Syscalls.c
    ${BDD_BOARD_LWIP_PROJECT_SOURCES}
    # SolidSyslog platform adapters arrive via the SolidSyslog::* pack umbrellas
    # linked below (S30.02 dogfood): LwipSocket (Address/Datagram/TcpStream/
    # Resolver), CmsisRtos (Mutex + SysUpTime), the TLS pack (TLS Stream +
    # HMAC/AES-GCM policies), LittleFs (file adapter). BddTargetLittleFsDisk.c is this target's
    # block device over the board's disk.
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetLittleFsDisk.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetErrorText.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetLittleFsMount.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetFreeRtosPipeline.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetKey.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetMessageSettings.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetOsPrimitives_CmsisRtos.c
    ${BDD_KERNEL_PROJECT_SOURCES}
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetClock.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetCustomSd.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetInteractive.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetIps.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetLanguage.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetMtlsConfig.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetSwitchConfig.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetBlockFiles.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetStoreSettings.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetTlsConfig.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetTlsInnerStream_LwipSocketTcp.c
    ${BDD_TLS_SOURCES}
    ${BAKED_PEM_HEADERS}
    $<TARGET_OBJECTS:solid_syslog_cmsis_lwip_upstream>
)

set_target_properties(SolidSyslogBddTargetCmsis PROPERTIES SUFFIX ".elf")

target_compile_options(SolidSyslogBddTargetCmsis PRIVATE
    ${BDD_BOARD_ARCH_FLAGS}
    -ffunction-sections
    -fdata-sections
    -fno-common
)

target_include_directories(SolidSyslogBddTargetCmsis PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}                       # lwipopts.h
    ${BDD_KERNEL_UPSTREAM_INCLUDES}                   # FreeRTOSConfig.h, kernel, CMSIS-RTOS2
    ${LWIP_SHARED_DIR}                                 # arch/cc.h
    ${BDD_BOARD_LWIP_INCLUDES}                        # the controller driver's headers
    ${BDD_BOARD_INCLUDES}                             # the device facts FreeRTOSConfig.h reads
    ${CMAKE_SOURCE_DIR}/Core/Interface                # SolidSyslog*.h
    ${CMAKE_SOURCE_DIR}/Core/Source                   # internal headers consumed by adapters
    # Platform/{LwipSocket,CmsisRtos,LittleFs}/{Interface,Source} and the TLS pack's include dirs
    # now arrive transitively from the SolidSyslog::* pack umbrellas linked below.
    ${BAKED_PEMS_DIR}                                 # BddBaked*.h (xxd -i output)
    ${BDD_TLS_INCLUDES}
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common            # BddTargetInteractive.h, BddTargetSwitchConfig.h, …
    ${LWIP_DIR}/src/include
    ${LWIP_CONTRIB_FREERTOS_DIR}/include              # arch/sys_arch.h (via lwip/sys.h)
)

# LittleFS's own lfs_util.h does not compile under -Wsign-conversion, which this
# project sets and upstream does not build with. SYSTEM silences its warnings
# without relaxing anything we compile - the same treatment
# docs/platforms/littlefs/setup.md tells an integrator on this bar to apply.
target_include_directories(SolidSyslogBddTargetCmsis SYSTEM PRIVATE
    ${LITTLEFS_PATH}
    ${BDD_TLS_SYSTEM_INCLUDES}
)

target_link_libraries(SolidSyslogBddTargetCmsis PRIVATE
    ${PROJECT_NAME}                                   # libSolidSyslog.a (cross-compiled)
    SolidSyslog::LwipSocket                           # lwIP Sockets adapters (need LWIP_SOCKET/LWIP_NETCONN/LWIP_DNS=1)
    SolidSyslog::CmsisRtos                            # Mutex + SysUpTime adapters (sources)
    SolidSyslog::LittleFs                             # LittleFS file adapter (sources)
    ${BDD_TLS_LIBRARIES}
)

# The wall clock this target hands its TLS library is seeded at configure time: the
# board has no RTC and no libc time() behind a syscall, and certificate
# validity only asks which side of a window the device is on. It drifts forward
# from the configure instant, which the ten-year test material absorbs.
string(TIMESTAMP SOLIDSYSLOG_BDD_BUILD_EPOCH "%s" UTC)

target_compile_definitions(SolidSyslogBddTargetCmsis PRIVATE
    ${BDD_TLS_DEFINITIONS}
    BDD_TARGET_BUILD_EPOCH=${SOLIDSYSLOG_BDD_BUILD_EPOCH}
)

target_link_options(SolidSyslogBddTargetCmsis PRIVATE
    ${BDD_BOARD_ARCH_FLAGS}
    -T${BDD_BOARD_LINKER_SCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-Map=$<TARGET_FILE:SolidSyslogBddTargetCmsis>.map
)

set_target_properties(SolidSyslogBddTargetCmsis PROPERTIES
    LINK_DEPENDS ${BDD_BOARD_LINKER_SCRIPT})
