/** @file
 *  A SecurityPolicy that appends a two-byte CRC-16 trailer to each stored
 *  record. Seal computes the CRC over the whole content; Open recomputes and
 *  compares. This is an unkeyed checksum — it catches accidental corruption
 *  (bit-rot, a truncated write) but is not tamper-evidence: anyone who edits a
 *  record can recompute a matching CRC. For tamper-evidence or confidentiality
 *  use a keyed policy. Being a checksum (not an AEAD), it ignores the record's
 *  header/body split and authenticates the content as one span. The instance is
 *  a shared stateless singleton, so it holds no pool slot and Destroy is a
 *  no-op. */
#ifndef SOLIDSYSLOGCRC16POLICY_H
#define SOLIDSYSLOGCRC16POLICY_H

#include "ExternC.h"

EXTERN_C_BEGIN

    /** An unkeyed at-rest integrity policy: it appends a CRC-16 trailer that
     *  catches accidental corruption of a stored record, but offers no defence
     *  against a deliberate edit (a tamperer can recompute the CRC). For
     *  tamper-evidence or confidentiality use a keyed policy (HmacSha256,
     *  AesGcm) instead. Returns a shared stateless instance; never NULL. */
    struct SolidSyslogSecurityPolicy* SolidSyslogCrc16Policy_Create(void);
    /** No-op: the policy is stateless and holds no pool slot. Present for
     *  lifecycle symmetry with the keyed policies. */
    void SolidSyslogCrc16Policy_Destroy(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGCRC16POLICY_H */
