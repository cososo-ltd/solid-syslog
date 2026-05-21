#include "CppUTest/TestHarness.h"

extern "C"
{
#include <mbedtls/version.h>
}

// Slice 1 (plumbing) skeleton: proves the integration-test binary links
// against real libmbedtls built from $MBEDTLS_DIR. The handshake-driving
// in-process TLS server lands in slice 3 alongside the certificate
// validation tests.

TEST_GROUP(SolidSyslogMbedTlsStreamIntegration){};

TEST(SolidSyslogMbedTlsStreamIntegration, BinaryLinksAgainstRealLibMbedTls)
{
    /* mbedtls_version_get_number() is a constant, side-effect-free symbol
     * present in every mbedTLS build — a successful link plus a return
     * value matching the expected major version (3.x) confirms the
     * integration scaffold pulls in the real library, not a fake. */
    const unsigned int major = (mbedtls_version_get_number() >> 24) & 0xFFU;
    LONGS_EQUAL(3, major);
}
