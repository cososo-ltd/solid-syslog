# mbedTLS adapter pack — shipped as sources, not a precompiled library.
#
# mbedTLS is header-configured: the integrator's mbedtls_config.h flows
# through upstream headers and changes the size/layout of mbedtls_ssl_context,
# mbedtls_x509_crt, mbedtls_pk_context, and the visible API surface.
# Compiling our adapter once into libSolidSyslog.a would silently break
# consumers that use a different mbedtls_config.h.
#
# Instead, this is an INTERFACE library — each consumer (Bdd/Targets/FreeRtos,
# Tests/MbedTls/*Test, downstream integrator apps) recompiles the adapter
# sources from Source/ with its own mbedtls_config.h on the include path.
# Mirrors Platform/FreeRtos/ and Platform/FatFs/ — see
# project_header_configured_platforms memory.
#
# The pack CARRIES its umbrella sources via target_sources(INTERFACE) (S30.02):
# a consumer that links SolidSyslog::MbedTls compiles the adapter .c files into
# its own target, seeing its own mbedtls_config.h — no per-file listing. The
# consumer still points at its own mbedTLS tree (mbedtls/*.h are consumer-located)
# and sets MBEDTLS_USER_CONFIG_FILE on its target.

add_library(SolidSyslogMbedTls INTERFACE)

# Umbrella = all mbedTLS adapters (they all compile against the consumer's
# mbedtls_config.h): the TLS Stream plus the two at-rest SecurityPolicies.
# Unused TUs dead-strip at link time (-Wl,--gc-sections).
target_sources(SolidSyslogMbedTls INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogMbedTlsStream.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogMbedTlsStreamStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogMbedTlsHmacSha256Policy.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogMbedTlsHmacSha256PolicyStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogMbedTlsAesGcmPolicy.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogMbedTlsAesGcmPolicyStatic.c
)

target_include_directories(SolidSyslogMbedTls INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogMbedTls*Private.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h, SolidSyslogMacros.h
)

target_link_libraries(SolidSyslogMbedTls INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::MbedTls ALIAS SolidSyslogMbedTls)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Interface/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
