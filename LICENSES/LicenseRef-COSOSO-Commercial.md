# COSOSO Commercial Licence Agreement

**SolidSyslog**

SPDX identifier: `LicenseRef-COSOSO-Commercial`

**Standard form, version 1.0, 2026-11-15.** The version a licensee holds is the one its
signed agreement incorporates. The revision table at the end records later versions.

This agreement has a body, which is the same for every licensee, and two schedules.
Schedule 1 says who is licensed and for what. Schedule 2 says what it costs and what
maintenance commits to. Only the schedules change between licensees.

---

**Between**

**Cozens Software Solutions Limited** ("COSOSO"), company number 09856828, registered
in England and Wales at Cawley Priory, South Pallant, Chichester, West Sussex, PO19 1SY

and

the **Licensee** named in Schedule 1.

**Effective date:** the date both parties have signed.

---

## 1. What the words mean

**Software**: the SolidSyslog library as COSOSO publishes it at
<https://github.com/cososo-ltd/solid-syslog>, meaning the Core library and the Platform
Packs in source form, the documentation, and every Release delivered under this agreement.

**Platform Pack**: a platform adapter COSOSO publishes as part of the Software.

**Release**: a version of the Software that COSOSO publishes with a version number.

**Licensed Versions**: every Release published on or before the last day of a
Maintenance Term the Licensee has paid for, including the first included term, and every
Security Fix for those Releases.

**Security Fix**: a Release whose purpose is to fix a vulnerability in the Software and
which adds no functionality.

**Product**: a device, firmware image, system or service that the Licensee or, under a
Group scope, a listed Affiliate, or someone acting for either of them, makes available to
anyone outside the Licensee and its listed Affiliates and which contains the Software or a
modified form of it.

**Product Line**: a family of Products marketed under one name and performing
substantially the same function, including its successors and variants. Schedule 1 names
each Product Line by family, function and the models known at signature. A Product sold
under a different name, or doing a different job, is a different Product Line.

**Licensed Scope**: the Product Lines, or the entity, or the group of entities, that
Schedule 1 says this licence covers.

**Affiliate**: an entity that controls, is controlled by, or is under common control
with the Licensee, where control means more than half the voting rights. An Affiliate is
covered only if Schedule 1 lists it.

**Distribute**: to supply, sell, lease, lend or otherwise make available to anyone
outside the Licensee, including by field trial, pilot or pre-production shipment.

**Maintenance Term**: twelve months for which the maintenance fee in Schedule 2 has
been paid, or the first twelve months, which the licence fee includes.

**Business Day**: a weekday other than a bank holiday in England.

**Declared Version**: a Release the Licensee has named in a declaration under clause 9
as shipping in a Product.

**Support Period**: for each Declared Version, five years from that Release's
publication date, or for as long as the Licensee continues to declare it, whichever is
longer, up to ten years from that publication date. A Security Fix's Support Period is
that of the Release it patches.

**Modifications**: changes the Licensee makes to the Software, including platform
adapters it writes against the Software's documented extension points.

## 2. What the Licensee may do

2.1 From the effective date, COSOSO grants the Licensee a non-exclusive, worldwide
licence, perpetual for each Licensed Version, to:

- use, copy, compile and modify the Licensed Versions to develop, build, test,
  maintain and support Products within the Licensed Scope;
- Distribute the Licensed Versions in compiled form only, as part of a Product within
  the Licensed Scope;
- let end users of those Products use the Software as part of the Product, under the
  Licensee's own terms.

2.2 Contract manufacturers, distributors, contractors and listed Affiliates may exercise
these rights on the Licensee's behalf, for the Licensee's Products only. Under a Group
scope a listed Affiliate may also exercise these rights for its own Products. The Licensee
is responsible for their compliance as if their acts were its own.

2.3 The Licensee owns its Modifications and need not disclose them. It may Distribute
them on the same terms as the Software. If it chooses to offer a Modification to COSOSO,
COSOSO may ask for it under a permissive inbound licence so that it can ship in every
tier.

2.4 Nothing here transfers ownership. The Software stays COSOSO's. The Licensee's
Products and its own integration code stay the Licensee's.

## 3. What the Licensee may not do

3.1 Distribute the Software other than inside a Product. In particular, not as a
library, SDK, source package or developer tool, and not as a logging service for other
people's products.

3.2 Distribute the Software in source form to anyone except those in clause 2.2, and
then only as needed to build Products.

3.3 Use the Software outside the Licensed Scope. Internal use outside the scope, such as
evaluating it for another product, is covered by the PolyForm Internal Use licence as
before. Shipping it in another Product needs that Product Line adding to Schedule 1 or a
move to a wider scope under clause 4.4.

3.4 Remove COSOSO's copyright notices or the SPDX headers from source it holds. Where a
Product's documentation or about screen lists third-party software, it includes the
notice: "Contains SolidSyslog, copyright Cozens Software Solutions Limited."

3.5 Use COSOSO's name or marks to endorse a Product without written consent. Saying
that a Product uses SolidSyslog is fine.

3.6 The Software's source is published at the repository in clause 1. That publication
grants no rights beyond this agreement, and COSOSO delivers source by no other channel.

## 4. Fees

4.1 The agreement starts when both parties have signed. COSOSO invoices the licence fee
in Schedule 2 on signature, payable within 30 days. The licence in clause 2 depends on
that payment: if it is not paid when due, the licence is suspended until it is. While
suspended, the Licensee does not Distribute the Software.

4.2 The maintenance fee for the first paid Maintenance Term is 20% of the licence fees
paid; later terms are indexed under clause 4.6. The first year is included in the licence
fee. Each Maintenance Term runs from one renewal date to the next and is invoiced 30 days
before it starts. If the fee is paid late, COSOSO's obligations under clause 5 are
suspended until payment and the term is not extended.

4.3 There is one renewal date per agreement: the anniversary of the effective date. A
Product Line added later has its own first year included; the whole months between the
end of that year and the next common renewal date are charged pro rata at the renewal
date immediately before that gap.

4.4 Licence fees paid in the previous twelve months count in full towards the licence fee
for a wider scope. Widening the scope, or adding a Product Line or Affiliate, is done by amending
Schedule 1 and Schedule 2 at the prices in force, subject to clause 4.5.

4.5 **Early adopter.** If this agreement's effective date is before Release 1.0.0, the
per-line rates, the Business and Group fees and the maintenance percentage in Schedule 2 stay
fixed for the life of the agreement, whatever COSOSO's list prices become. They apply to
renewals, additional Product Lines, Affiliates and any wider scope. Clause 4.6 still
applies.

4.6 At each renewal after the first paid Maintenance Term, the previous term's maintenance
fee is increased by the most recent twelve-month change in the UK Consumer Prices Index
(all items) that the Office for National Statistics has published at the date of the
renewal invoice, never downwards and by no more than 5%. A Product Line added later joins
the base at 20% of its licence fee at its first charged renewal. If the index is
discontinued, its official successor applies.

4.7 Fees exclude VAT, which is added where it applies. They are payable in pounds
sterling, in full, without set-off, deduction or withholding. If the law requires a
deduction or withholding, the Licensee pays enough extra that COSOSO receives the full
amount, and both parties cooperate on any treaty relief. Overdue sums carry interest at 8%
a year above the Bank of England base rate, accruing daily from the due date until paid.

## 5. Maintenance

5.1 During a Maintenance Term COSOSO shall:

- make every Release available to the Licensee as it is published, including new
  platform packs;
- keep the Platform Packs the Licensee has declared building against new releases of the
  TLS libraries, operating systems, network stacks and filesystems they support, within
  the upstream version range each Platform Pack's documentation states it tracks; a new
  major version of an upstream is platform work under clause 5.2. Where
  a platform cannot be built or run in COSOSO's own CI, as its documentation states,
  the commitment is to keep the Platform Pack building and passing its unit tests in CI;
  verifying it on the platform itself needs the Licensee's toolchain or hardware and
  is done with the Licensee's cooperation, following the target checklist the
  documentation publishes;
- provide a named technical contact for questions on integrating and using the
  Software, acknowledging within two Business Days;
- acknowledge a reported defect or vulnerability within two Business Days and triage
  it within five Business Days;
- for a vulnerability it assesses as High or Critical under CVSS v3.1, deliver a Security
  Fix or a documented workaround within 30 days of triage;
- deliver with every Release the published evidence: test report, coverage, SBOM,
  signed source hash and offline documentation.

5.2 Maintenance does not include engineering work on the Licensee's product, adapters
for platforms COSOSO does not ship, or support of Modifications. These are provided
under a separate statement of work, if agreed.

5.3 COSOSO is a single maintainer. Any time limit in clauses 5 or 6 is extended by the
period in which illness, bereavement or events outside COSOSO's reasonable control prevent
it from acting, provided COSOSO tells the Licensee as soon as it can. If a High or
Critical fix is delayed by more than 90 days in total, clause 5.5 applies.

5.4 Maintenance lapsing does not end this agreement. If maintenance lapses, the Licensee
keeps every right in clause 2 over the Licensed Versions and keeps shipping. It receives no new Releases or support. Reinstating
maintenance pays the fees for the lapsed period, up to two years.

5.5 If COSOSO misses the 30-day commitment in clause 5.1 for a High or Critical
vulnerability and does not put it right within 30 days of written notice, the Licensee
may end the current Maintenance Term and receive a pro-rata refund of the maintenance fee
for that term, treating the included first term as paid at the rate in clause 4.2.

## 6. Security fixes, whether or not maintenance is paid

6.1 Throughout the Support Period of each Declared Version, COSOSO shall fix, or publish
a documented workaround for, vulnerabilities in the Software as released, free of charge,
following its published security policy for timescale, disclosure and advisories. The
response times in clause 5.1 apply only during a Maintenance Term. COSOSO tells the
Licensee contact of a High or Critical vulnerability no later than it publishes the
advisory.

6.2 A Security Fix is issued as a patch release of the Declared Version where that is
needed to apply it, so that the Licensee is not required to take a later Release to
receive a security fix.

6.3 This clause reflects both parties' intention that the Licensee can meet its
obligations under Regulation (EU) 2024/2847, the Cyber Resilience Act, for the Software
as a component of its Products. It is not a warranty that the Software or this agreement
meets any requirement of that Regulation. The Licensee, as manufacturer of its Products,
is responsible for its own compliance, and reports to COSOSO's published security address
any vulnerability it finds in the Software before disclosing it elsewhere.

6.4 Clause 6 covers the Software as COSOSO released it. It does not cover
Modifications, the Licensee's integration or configuration, or vulnerabilities in
third-party libraries the Licensee links against. Following a third-party library's
own breaking update is maintenance under clause 5, not a Security Fix.

## 7. Warranties and indemnity

7.1 COSOSO warrants that it owns the Software, that the Core library and the Platform
Packs contain no third-party source code other than as declared in the SBOM, and that to
its knowledge the Software does not infringe anyone's copyright, database right or trade
mark.

7.2 COSOSO warrants that for 90 days from its publication at the repository each Release
performs materially as documented. The remedy is repair or replacement, or failing that a
refund: for the first Release delivered under this agreement, of the licence fee, against
the Licensee ceasing to use the Software; for a later Release, of the maintenance fee for
the Maintenance Term in which it was delivered, treating the included first term as paid
at the rate in clause 4.2. This is the Licensee's only remedy for breach of this warranty.

7.3 COSOSO shall defend the Licensee against a third-party claim that the unmodified
Software infringes a copyright, database right or trade mark, and pay damages and costs
finally awarded or agreed in a settlement COSOSO approves, provided the Licensee notifies
COSOSO promptly, lets it control the defence, gives reasonable help, and does not settle
or admit liability without COSOSO's consent. COSOSO may obtain a right to continue, change
the Software so it no longer infringes, or refund the licence fees and end the agreement.
This does not cover patents or trade secrets, Modifications, combinations with other
software where the claim would not have arisen from the Software alone, or use outside the
Licensed Scope. COSOSO's total liability under this clause, defence costs included, is
twice the licence fees paid. Clause 8.2 does not apply to sums payable under this clause.

7.4 Apart from clauses 7.1 to 7.3 the Software comes with no other warranty, express or
implied, and is not warranted error-free or uninterrupted. It is not certified to any
safety standard. The Licensee is responsible for
qualifying it for its Products, including any safety or security certification.

## 8. Liability

8.1 Nothing limits liability for death or personal injury caused by negligence, for
fraud, or for anything that cannot lawfully be limited.

8.2 Subject to 8.1, neither party is liable for loss of profit, revenue, data or
goodwill, or for indirect or consequential loss. This does not apply to the Licensee's
breach of clauses 2, 3 or 9, for which the licence fees COSOSO would have charged for the
use actually made are recoverable as a debt.

8.3 Subject to 8.1 and 8.2, each party's total liability arising from or connected with
this agreement, however arising, including negligence, in any twelve months is limited to
the greater of the fees paid by the Licensee in the twelve months before the claim and
the licence fees paid, except that clause 7.3 has its own cap and the cap does not apply
to the Licensee's breach of clauses 2, 3 or 9, to unpaid fees, or to either party's breach
of clause 12.1.

## 9. Records and declaration

9.1 The Licensee keeps records showing which Products contain the Software and which
Releases they contain.

9.2 At each renewal date, and otherwise on request no more than once a year, the
Licensee confirms in writing the Product Lines shipping under this agreement, the
Affiliates exercising rights, and the Releases each Product contains. These are the
Declared Versions. No on-site audit right is taken. Declarations continue for as long as
any Support Period runs, whether or not maintenance is in force; for this purpose the
renewal date is the anniversary of the effective date.

9.3 In a software bill of materials or licence declaration for a Product, the Licensee
records `LicenseRef-COSOSO-Commercial` as the concluded licence for the Software and
need not carry COSOSO's published multi-licence expression.

## 10. Ending the agreement

10.1 The agreement runs from the effective date until ended.

10.2 Either party may end it for a material breach not put right within 30 days of
written notice.

10.3 If it ends for the Licensee's breach, clause 2 ends, except that for six months the
Licensee may Distribute Products already built. Products already supplied are unaffected and may
still receive security updates, and their end users keep the right in clause 2.1 to use
the Software as part of the Product.

10.4 If it ends for any other reason, the licence over the Licensed Versions survives,
except where COSOSO ends it under clause 7.3, when the licence ends on refund and the
Licensee stops Distributing the Software within 90 days. Products already supplied are
unaffected.

10.5 Clauses 1, 3, 4.7 for sums accrued, 5.4, 7, 8, 9, 10, 11 and 12 survive. Clause 6
survives for the rest of each Support Period, unless the agreement ended for the
Licensee's breach, when COSOSO may provide Security Fixes but need not.

## 11. Continuity and assignment

11.1 COSOSO has stopped maintaining the Software if it enters liquidation or
administration, is struck off or dissolved, or announces in writing or in the repository
that it has stopped. If no successor has agreed in writing, within six months of that, to
perform clauses 5, 6 and 11 for the Licensee, the Licensee holds every Release published
by that date under the Apache License, Version 2.0. COSOSO grants that licence now, to take
effect on that date, and no further act is needed. The Licensee's rights under clause 2
are unaffected either way. The Software is delivered in source form, so there is no
escrow. Nothing in clause 5.3 extends the periods in this clause.

11.2 COSOSO may assign this agreement to a successor that agrees in writing to perform
clauses 5, 6 and 11. The Licensee may assign it, on written notice, to a listed Affiliate.
Where the Licensee sells a Product Line, that line is removed from Schedule 1 and COSOSO
grants the buyer an agreement on these terms for it, on the buyer agreeing in writing to
be bound. Otherwise neither party assigns without the other's consent, not to be
unreasonably withheld.

## 12. General

12.1 Confidential information is both Schedules and anything marked or obviously
confidential. It excludes what is public, already known to the recipient, or must be
disclosed by law or a regulator. Each party keeps the other's confidential information
confidential for five years after the agreement ends. COSOSO names the Licensee as a
customer only with written consent.

12.2 This agreement with its schedules and any statement of work is the whole agreement
for the Licensed Scope. Nothing in a purchase order, supplier portal or standard
purchasing terms changes it. Neither party has relied on any statement not in it, except
a fraudulent one. Within the Licensed Scope it replaces the PolyForm
licences; outside it, PolyForm Internal Use continues to cover internal use.

12.3 Changes only in writing, signed by both parties.

12.4 Notices in writing to the registered office or the email address in Schedule 1.
Email is received when sent unless it bounces; post two Business Days after posting. A
notice of breach or termination also goes by post.

12.5 If a provision is unenforceable the rest stands. No third party has rights under
the Contracts (Rights of Third Parties) Act 1999 except end users under clause 10.3, and the
parties may change or end this agreement without any end user's consent. Counterparts are
fine.

12.6 English law. The courts of England and Wales have exclusive jurisdiction.

12.7 Each party handles the other's contact details as a controller under its own privacy
notice; neither processes personal data for the other.

---

## Schedule 1: who and what

| Field | Entry |
|---|---|
| Licensee | [Legal name], company number [number], registered office [address] |
| Licensed Scope | One of: **Product Line** / **Business** (all Products of the Licensee) / **Group** (all Products of the Licensee and the Affiliates listed) |
| Product Lines | For a Product Line scope: for each line, the family name, its function, the current models as examples, and any exclusions. Example: "The [Family] range of [function], including the models known at signature as [models], their successors and variants. Excludes [none]." |
| Affiliates | For a Group scope: registered name, company number and jurisdiction of each. Otherwise: none |
| COSOSO contact | David Cozens, [email] |
| Licensee contact | [name], [email] |
| Statements of work | [list, or none] |

## Schedule 2: fees and commitments

| Field | Entry |
|---|---|
| Licence fee | £[amount], made up of [n] Product Lines at £[rate] / Business licence / Group licence |
| Maintenance fee | 20% of licence fees paid per year; first year included; renewal date [date]; indexed per clause 4.6 |
| Early adopter | Yes / No (clause 4.5) |
| Response commitments | Acknowledgement 2 Business Days; triage 5 Business Days; High or Critical fix or workaround 30 days from triage (clause 5.1) |
| Support Period | Clause 1, for each Declared Version |

## Revisions

| Version | Date | Change |
|---|---|---|
| 1.0 | 2026-11-15 | First standard form. |

---

**Signed for COSOSO**: David Cozens, Director. Date:

**Signed for the Licensee**: Name: Position: Date:
