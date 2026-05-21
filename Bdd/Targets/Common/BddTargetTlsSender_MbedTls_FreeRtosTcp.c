/* MbedTLS-over-FreeRtosTcpStream variant of the BDD TLS sender.
 *
 * Slice 6a (this file): scaffolding only. Returns a no-op sender so the
 * FreeRTOS BDD target compiles and links with mbedTLS wired into the build,
 * proving the platform / config / CMake stack is healthy before slice 6b
 * lands the real entropy + DRBG + PEM-parse + StreamSender wiring.
 *
 * Once 6b lands, this file will compose:
 *   - SolidSyslogFreeRtosTcpStream                (inner TCP transport)
 *   - SolidSyslogMbedTlsStream                    (TLS, with handles passed
 *                                                  from main.c's CTR_DRBG +
 *                                                  parsed CA / client cert)
 *   - SolidSyslogStreamSender                     (octet-framed RFC 6587)
 *
 * Mirroring BddTargetTlsSender_OpenSsl_PosixTcp.c on the POSIX target. The
 * mbedTLS adapter takes pre-built handles rather than file paths (no
 * MBEDTLS_FS_IO on this target), so the demo PEMs are baked into the ELF
 * via xxd -i in slice 6b and parsed at first BddTargetTlsSender_Create call.
 */

#include "BddTargetTlsSender.h"
#include "SolidSyslogSenderDefinition.h"

#include <stddef.h>

struct SolidSyslogResolver;

static bool MbedTlsBddSender_Send(struct SolidSyslogSender* self, const void* buffer, size_t size)
{
    (void) self;
    (void) buffer;
    (void) size;
    /* Slice 6a placeholder: drop on the floor. Real implementation in 6b. */
    return false;
}

static void MbedTlsBddSender_Disconnect(struct SolidSyslogSender* self)
{
    (void) self;
}

static struct SolidSyslogSender mbedTlsBddSender = {MbedTlsBddSender_Send, MbedTlsBddSender_Disconnect};

struct SolidSyslogSender* BddTargetTlsSender_Create(struct SolidSyslogResolver* resolver, bool mtls)
{
    (void) resolver;
    (void) mtls;
    return &mbedTlsBddSender;
}

void BddTargetTlsSender_Destroy(void)
{
}
