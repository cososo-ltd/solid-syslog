# Template options — scratch

Not part of the documentation. A scratch page for choosing between layout
options in the real theme, deleted before this branch is finished.

<!-- markdownlint-disable MD033 — the whole point of this page is rendered samples. -->

<style>
.mk { border: 1px dashed #d0d0d8; border-radius: 4px; padding: 1rem 1.2rem; margin: .6rem 0 1.6rem; }
.mk .h1 { font-family: Montserrat, sans-serif; font-weight: 700; font-size: 1.9rem; margin: 0 0 .2rem; }
.mk .body { color: #4a4a55; margin: .5rem 0 0; max-width: 42rem; }

/* A — quiet link row under the title */
.opt-row { font-size: .78rem; letter-spacing: .01em; margin: 0; }
.opt-row a { text-decoration: none; border-bottom: 1px solid rgba(62,33,153,.35); }
.opt-row .sep { color: #b0b0bb; margin: 0 .55rem; }

/* B — Material buttons (real .md-button, shown for weight comparison) */

/* C — title suffix */
.opt-suffix { font-size: .8rem; font-weight: 400; vertical-align: .35em; margin-left: .5rem; }

/* D — chips with glyphs */
.chip { display: inline-flex; align-items: center; gap: .38rem; font-size: .76rem;
        padding: .2rem .6rem .2rem .45rem; border-radius: 999px; text-decoration: none;
        background: #eef0fa; color: #2f2a55; border: 1px solid #d5d9f0; }
.chip:hover { background: #e2e6f7; }
.chip + .chip { margin-left: .4rem; }

/* Banner options for generated API pages */
.ban-b { display: flex; align-items: center; gap: .45rem; font-size: .78rem;
         color: #4a4a55; margin: 0 0 1.1rem; }
.ban-b a { text-decoration: none; border-bottom: 1px solid rgba(62,33,153,.35); }

.ban-c { display: inline-flex; align-items: center; gap: .5rem;
         background: #cdd4f5; border: 1px solid #9aa6e0; color: #23264a;
         padding: .3rem .7rem .3rem .55rem; border-radius: 3px;
         box-shadow: 1px 2px 3px rgba(0,0,0,.16); font-size: .78rem;
         text-decoration: none; margin: 0 0 1.1rem; }
.ban-c:hover { background: #bcc5f0; }

.ban-d { display: flex; align-items: center; gap: .4rem; font-size: .72rem;
         text-transform: uppercase; letter-spacing: .07em; color: #6a6a78;
         margin: 0 0 1.1rem; }
.ban-d a { color: #3e2199; text-decoration: none; font-weight: 600; }

.mk .stub { color: #9a9aa8; font-size: .82rem; margin-top: .9rem; }
</style>

<svg width="0" height="0" style="position:absolute" aria-hidden="true"><defs>
<g id="uml-comp">
  <rect x="4.5" y="2.5" width="10" height="11" rx="1" fill="#cdd4f5" stroke="#7f8dd4" stroke-width="1"/>
  <rect x="1.5" y="5" width="5.5" height="2.6" rx=".4" fill="#fff" stroke="#7f8dd4" stroke-width="1"/>
  <rect x="1.5" y="8.9" width="5.5" height="2.6" rx=".4" fill="#fff" stroke="#7f8dd4" stroke-width="1"/>
</g>
</defs></svg>

## Part 1 — getting to API reference and Setup from a platform page

Both are currently short sections at the bottom. Four ways to move them to the
top. Each sample shows the real first screen of the Mbed TLS page.

### A — quiet link row beneath the title

<div class="mk">
  <p class="h1">Mbed TLS</p>
  <p class="opt-row"><a href="../../api/group__platform__mbedtls/">API reference</a><span class="sep">·</span><a href="../../platforms/mbedtls/setup/">Setup</a></p>
  <p class="body"><code>Platform/MbedTls/</code> wraps Mbed TLS for TLS transport and keyed at-rest cryptography on embedded targets. It fills the Stream role with TLS and the SecurityPolicy role for at-rest integrity and confidentiality.</p>
  <p class="stub">## What it ships …</p>
</div>

Lightest touch. Reads as metadata about the page rather than as content, which
is what it is. Risk: quiet enough to be missed.

### B — Material buttons

<div class="mk">
  <p class="h1">Mbed TLS</p>
  <p><a class="md-button" href="../../api/group__platform__mbedtls/">API reference</a> <a class="md-button" href="../../platforms/mbedtls/setup/">Setup</a></p>
  <p class="body"><code>Platform/MbedTls/</code> wraps Mbed TLS for TLS transport and keyed at-rest cryptography on embedded targets. It fills the Stream role with TLS and the SecurityPolicy role for at-rest integrity and confidentiality.</p>
  <p class="stub">## What it ships …</p>
</div>

Impossible to miss, and native to the theme — no new CSS. But it pushes the
first paragraph down a long way, and two buttons on ten pages is a lot of
furniture for what is really navigation.

### C — title suffix, your idea 2

<div class="mk">
  <p class="h1">Mbed TLS <span class="opt-suffix"><a href="../../api/group__platform__mbedtls/">API</a> · <a href="../../platforms/mbedtls/setup/">Setup</a></span></p>
  <p class="body"><code>Platform/MbedTls/</code> wraps Mbed TLS for TLS transport and keyed at-rest cryptography on embedded targets. It fills the Stream role with TLS and the SecurityPolicy role for at-rest integrity and confidentiality.</p>
  <p class="stub">## What it ships …</p>
</div>

Costs no vertical space at all. Two problems: it lands in the table of contents
and the browser tab title unless worked around, and "API" alone is terse to the
point of cryptic for a first-time reader.

### D — chips with glyphs

<div class="mk">
  <p class="h1">Mbed TLS</p>
  <p><a class="chip" href="../../api/group__platform__mbedtls/"><svg width="16" height="16" viewBox="0 0 16 16" aria-hidden="true"><use href="#uml-comp"/></svg>API reference</a><a class="chip" href="../../platforms/mbedtls/setup/"><svg width="16" height="16" viewBox="0 0 16 16" aria-hidden="true"><path d="M10.6 1.6a4 4 0 0 0-3.5 5.9L1.9 12.7a1.2 1.2 0 0 0 1.7 1.7l5.2-5.2a4 4 0 0 0 4.9-5.2l-2 2-1.6-.4-.4-1.6 2-2a4 4 0 0 0-1.1-.4z" fill="#cdd4f5" stroke="#7f8dd4" stroke-width="1" stroke-linejoin="round"/></svg>Setup</a></p>
  <p class="body"><code>Platform/MbedTls/</code> wraps Mbed TLS for TLS transport and keyed at-rest cryptography on embedded targets. It fills the Stream role with TLS and the SecurityPolicy role for at-rest integrity and confidentiality.</p>
  <p class="stub">## What it ships …</p>
</div>

Between A and B in weight, and the component glyph is the same one Part 2 uses,
so the two places agree visually. New CSS to own, and the wrench is decorative
rather than meaningful.

---

## Part 2 — the platform banner on a generated API page

Same four, against the current one. These sit at the very top of
`SolidSyslogMbedTlsStream_8h`, above the relationship diagram.

### Current — full-width admonition

<div class="mk">
  <div class="admonition info"><p class="admonition-title">Part of the <a href="../../platforms/mbedtls/">Mbed TLS</a> platform</p>
  <p>What the pack ships, what your build must provide, and the obligations it leaves to you.</p></div>
  <p class="stub">SolidSyslogMbedTlsStream.h — File Reference …</p>
</div>

### B — one quiet line, left justified

<div class="mk">
  <p class="ban-b"><svg width="16" height="16" viewBox="0 0 16 16" aria-hidden="true"><use href="#uml-comp"/></svg>Part of the <a href="../../platforms/mbedtls/">Mbed TLS</a> platform</p>
  <p class="stub">SolidSyslogMbedTlsStream.h — File Reference …</p>
</div>

### C — post-it chip, in the kit's adapter blue

<div class="mk">
  <a class="ban-c" href="../../platforms/mbedtls/"><svg width="16" height="16" viewBox="0 0 16 16" aria-hidden="true"><use href="#uml-comp"/></svg>Mbed TLS platform</a>
  <p class="stub">SolidSyslogMbedTlsStream.h — File Reference …</p>
</div>

Blue is already the diagram kit's colour for "a backend that realises a role",
which is exactly what a platform adapter is — so this borrows a vocabulary the
site has rather than inventing one.

### D — eyebrow label

<div class="mk">
  <p class="ban-d"><svg width="14" height="14" viewBox="0 0 16 16" aria-hidden="true"><use href="#uml-comp"/></svg>Platform <a href="../../platforms/mbedtls/">Mbed TLS</a></p>
  <p class="stub">SolidSyslogMbedTlsStream.h — File Reference …</p>
</div>

Smallest of the four. Reads as a category label, which is accurate.

<!-- markdownlint-enable MD033 -->

---

## Part 3 — two wording decisions, no mock needed

**"Pack" goes.** I introduced it to avoid repeating "platform" and it reads as
jargon for no gain. The page is the platform, so: *"Every class in this
platform"*, or name it — *"Every class in Mbed TLS"*. The second is better
still, because it works when read out of context.

**When is a class name a link?** Right now it is a link in the *What it ships*
table and plain code in prose, which is the inconsistency you spotted. The rule
I would propose: the table is the canonical linked index of what the platform
ships, and every mention in prose is plain code. One place to click, no
judgement call per sentence, and no page full of blue.

The alternative — link the first mention anywhere — needs a decision on every
paragraph and drifts the moment anyone edits.
