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

add_library(SolidSyslogMbedTls INTERFACE)

# No adapter sources at S08.07 slice 1 — first content
# (SolidSyslogMbedTlsStream.h plus the three-TU split) lands in the
# slice-1 green step. The INTERFACE target exists so the directory and
# CMake wiring are in place from the plumbing slice.

target_include_directories(SolidSyslogMbedTls INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Interface/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
