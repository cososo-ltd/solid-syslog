/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The SecurityPolicy vtable (SealRecord / OpenRecord) - the at-rest
 *  integrity/confidentiality contract an implementor fills in (the SecurityPolicy
 *  extension point). SealRecord authenticates (AEAD policies also encrypt) and
 *  writes a trailer on the way in; OpenRecord verifies (and decrypts) on replay,
 *  failing closed so a tampered or unkeyed record is dropped rather than replayed. */
#ifndef SOLIDSYSLOGSECURITYPOLICYDEFINITION_H
#define SOLIDSYSLOGSECURITYPOLICYDEFINITION_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** One record presented to a security policy. The Content span is split as
     *    Content[0 .. HeaderLength)             associated data, authenticated
     *                                           but never encrypted (the
     *                                           cleartext header the store needs
     *                                           to locate the record on replay)
     *    Content[HeaderLength .. ContentLength) body, authenticated; an AEAD
     *                                           policy also encrypts it in place
     *  and Trailer is the policy-owned span of TrailerSize bytes. MAC and
     *  checksum policies authenticate the whole Content and ignore the split;
     *  AEAD policies treat the header as associated data and encrypt the body.
     *
     *  Passed by const pointer: a policy may not repoint the fields, but the
     *  member pointers are non-const so it can write through Content (in-place
     *  encrypt/decrypt) and Trailer (seal) per the contract. */
    struct SolidSyslogSecurityRecord
    {
        uint8_t* Content;
        uint16_t ContentLength;
        uint16_t HeaderLength;
        uint8_t* Trailer; /**< Written by SealRecord, read back and consumed by OpenRecord. */
    };

    /** The at-rest integrity/confidentiality contract the block store applies to
     *  every record: SealRecord on the way in (before the record is stored),
     *  OpenRecord on replay-read (before the record is handed back). A record
     *  round-trips through exactly one policy; the two calls must agree on the
     *  trailer layout.
     *
     *  Reference implementations span the protection scale: the Null policy is a
     *  pass-through that adds no trailer and never rejects (no integrity, no
     *  confidentiality); Crc16 is an unkeyed checksum catching accidental
     *  corruption; the HmacSha256 and AesGcm policies are keyed and fetch their
     *  key on demand from an integrator callback, failing closed (Seal returns
     *  false, nothing is stored) if the key is unavailable. */
    struct SolidSyslogSecurityPolicy
    {
        /** Bytes SealRecord appends per record; the store reserves this and
         *  treats it as opaque. Zero for the Null policy. */
        uint16_t TrailerSize;
        /** Authenticate (and, for an AEAD policy, encrypt in place) @p record and
         *  write its trailer. @retval false the seal could not be produced (e.g.
         *  key unavailable); the store drops the record rather than storing it
         *  unprotected. */
        bool (*SealRecord)(struct SolidSyslogSecurityPolicy* self, const struct SolidSyslogSecurityRecord* record);
        /** Verify @p record against its trailer (and, for an AEAD policy, decrypt
         *  the body in place). @retval false integrity check failed: tampering or
         *  corruption detected, or the open could not run (e.g. key unavailable);
         *  the store discards the record rather than replaying it. */
        bool (*OpenRecord)(struct SolidSyslogSecurityPolicy* self, const struct SolidSyslogSecurityRecord* record);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSECURITYPOLICYDEFINITION_H */
