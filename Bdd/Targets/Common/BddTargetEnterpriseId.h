#ifndef BDDTARGETENTERPRISEID_H
#define BDDTARGETENTERPRISEID_H

/* The enterprise number used by the example apps and asserted verbatim by the
   BDD scenario. RFC 5612 reserves 32473 for use in examples, which is what this
   is; a real deployment substitutes its own IANA-registered number. RFC 5424
   §7.2.2 wants the number alone rather than the 1.3.6.1.4.1 arc it sits under. */
#define BDD_TARGET_ENTERPRISE_ID "32473"

#endif /* BDDTARGETENTERPRISEID_H */
