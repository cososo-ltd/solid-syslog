# mbedTLS adapter pack. An INTERFACE library rather than a precompiled one: the
# integrator's mbedtls_config.h changes the layout of mbedtls_ssl_context,
# mbedtls_x509_crt and mbedtls_pk_context and the visible API, so each consumer
# compiles these sources against their own config. Linking SolidSyslog::MbedTls
# is enough — the pack carries its sources. The consumer points at their own
# mbedTLS tree and sets MBEDTLS_USER_CONFIG_FILE on their target.

add_library(SolidSyslogMbedTls INTERFACE)

# Umbrella = all mbedTLS adapters (they all compile against the consumer's
# mbedtls_config.h): the TLS Stream plus the two at-rest SecurityPolicies.
# Unused TUs dead-strip at link time (-Wl,--gc-sections).
target_sources(SolidSyslogMbedTls INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogMbedTlsNullCredentials.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogMbedTlsHandleCredentials.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogMbedTlsHandleCredentialsStatic.c
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

solidsyslog_install_public_headers()
