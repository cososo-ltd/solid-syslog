# wolfSSL adapter pack. An INTERFACE library rather than a precompiled one: the
# integrator's wolfSSL build options (wolfssl/options.h from ./configure, or a
# user_settings.h under WOLFSSL_USER_SETTINGS) change the layout of every
# wolfSSL context and the visible API, so each consumer compiles these sources
# against their own build. Linking SolidSyslog::WolfSsl is enough - the pack
# carries its sources. The consumer puts their wolfSSL include directory on
# their target, and defines WOLFSSL_USER_SETTINGS there if that is how their
# library was built.

add_library(SolidSyslogWolfSsl INTERFACE)

# Umbrella = all wolfSSL adapters (they all compile against the consumer's
# build options). Unused TUs dead-strip at link time (-Wl,--gc-sections).
target_sources(SolidSyslogWolfSsl INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogWolfSslHmacSha256Policy.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogWolfSslHmacSha256PolicyStatic.c
)

target_include_directories(SolidSyslogWolfSsl INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogWolfSsl*Private.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h, SolidSyslogMacros.h
)

target_link_libraries(SolidSyslogWolfSsl INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::WolfSsl ALIAS SolidSyslogWolfSsl)

solidsyslog_install_public_headers()
