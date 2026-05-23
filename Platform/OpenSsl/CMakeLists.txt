# OpenSSL was discovered (or explicitly requested) at the top-level CMakeLists.
# Platform/OpenSsl is only entered when SOLIDSYSLOG_OPENSSL is ON, so we can
# rely on the OPENSSL_INCLUDE_DIR variable being set.

target_sources(${PROJECT_NAME} PRIVATE
    Source/SolidSyslogTlsStream.c
    Source/SolidSyslogTlsStreamMessages.c
    Source/SolidSyslogTlsStreamStatic.c
)

# Headers at compile time; libssl is linked by consumers (real linkage) or
# replaced by OpenSslFake in tests — not declared as a library link here.
target_include_directories(${PROJECT_NAME}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    PRIVATE ${OPENSSL_INCLUDE_DIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Interface/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
