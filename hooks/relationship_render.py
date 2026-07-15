"""Render a post-it "realised by" diagram for one vtable role.

Parametric, driven by simple rules so the general case behaves:
  * <= 4 backends -> one row; >= 5 -> two staggered rows (the bottom row is
    offset half a pitch so its droppers thread the gaps of the top row).
  * the name font scales down to fit the longest backend name in the set.
  * the Null object is drawn grey; a small override marks backends that also
    wrap an inner instance of their own role (the decorator case).

Output is a self-contained inline ``<svg>`` string. Its ``<style>`` is scoped by
the svg's id so nothing leaks into the page. Each sticky links to its API page.
"""

# Page-relative. Every diagram is injected on an api/<name>_8h/ page, so a sibling
# API page is one level up — "../<page>/". Keeps node links correct under any
# deployment prefix and under `mkdocs serve`, not just the /solid-syslog/ base.
BASE_URL = ".."

# fill, stroke, text
_GREEN = ("#bfe3c0", "#6fae74", "#21401f")
_PERI = ("#cdd4f5", "#8f9bd6", "#15324f")
_GREY = ("#e4e4e2", "#b0b0a8", "#45453d")
_PINK = ("#f6c6c6", "#cf8a8a", "#4a2c2c")
_YELLOW = ("#ffe7a3", "#b89a3c", "#3a3320")
_PKG = "#3a4a80"
_PKG_NULL = "#6a6a60"

_HAND = "'Segoe Print','Bradley Hand','Patrick Hand',cursive"
_UI = "'Segoe UI',sans-serif"

_STICKY_W, _STICKY_H, _GAP = 152, 60, 26
_PITCH = _STICKY_W + _GAP
_BASE_W, _BASE_H = 200, 66
_MARGIN, _BASE_Y = 22, 16
_BUS_GAP, _ROW_GAP = 38, 28
_ROT = [-2, 1.5, -1, 2, -1.5, 1, -0.5, 2, -1, 1.5]

# backends that also wrap an injected instance of their own role (decorator)
_WRAPS = {"Stream": {"TlsStream", "MbedTlsStream"}}


def _fit(text_len, inner, cap):
    return round(max(8.0, min(cap, inner / (text_len * 0.6))), 1)


def _sticky(cx, y, impl, index, name_font, pkg_font, role, wraps):
    fill, stroke, text = _GREY if impl["is_null"] else _PERI
    pkg_color = _PKG_NULL if impl["is_null"] else _PKG
    pkg = impl["package"] + (" · no-op" if impl["is_null"] else "")
    left = cx - _STICKY_W / 2
    cy = y + _STICKY_H / 2
    rot = _ROT[index % len(_ROT)]
    out = ['<a href="{}/{}/" target="_top" class="node">'.format(BASE_URL, impl["page"]),
           f'<g transform="rotate({rot} {cx} {cy})">']
    if impl["name"] in wraps:
        ty = y + _STICKY_H - 6
        out.append(f'<g filter="url(#sh)"><rect x="{cx - 34}" y="{ty}" width="68" height="22" rx="2" '
                   f'fill="{_PINK[0]}" stroke="{_PINK[1]}" stroke-width="1.2"/></g>')
        out.append(f'<text x="{cx}" y="{ty + 15}" text-anchor="middle" font-size="8.5" font-family="{_UI}" '
                   f'fill="{_PINK[2]}">wraps a {role}</text>')
    out.append(f'<g filter="url(#sh)"><rect class="card" x="{left}" y="{y}" width="{_STICKY_W}" height="{_STICKY_H}" '
               f'fill="{fill}" stroke="{stroke}" stroke-width="1.4"/></g>')
    out.append('<text x="{}" y="{}" text-anchor="middle" font-size="{}" font-family="{}" '
               'font-weight="bold" fill="{}">{}</text>'.format(cx, y + 26, name_font, _HAND, text, impl["name"]))
    out.append(f'<text x="{cx}" y="{y + 45}" text-anchor="middle" font-size="{pkg_font}" font-style="italic" '
               f'font-family="{_UI}" fill="{pkg_color}">{pkg}</text>')
    out.append("</g></a>")
    return "".join(out)


# ---------------------------------------------------------------------------
# Reverse view: one implementer, the role it realises (up) + the roles it uses
# (down). Injected on each concrete implementer's own API page.
# ---------------------------------------------------------------------------

_IF_W, _IF_H = 196, 60
_SUB_W, _SUB_H = 158, 64
_COL_GAP = 28
_V_REALISES, _V_USES = 46, 52

_REVERSE_DEFS = (
    '<defs>'
    # userSpaceOnUse (not the default objectBoundingBox): an all-vertical connector
    # group has a zero-width bbox, which collapses an objectBoundingBox filter region
    # to zero and makes browsers drop the element. Tying the region to the viewport
    # keeps it non-degenerate for leaf/self-wrap diagrams whose only line is vertical.
    '<filter id="rough" filterUnits="userSpaceOnUse" x="-5%" y="-5%" width="110%" height="110%">'
    '<feTurbulence type="fractalNoise" baseFrequency="0.012" numOctaves="2" seed="7" result="n"/>'
    '<feDisplacementMap in="SourceGraphic" in2="n" scale="3"/></filter>'
    '<filter id="sh" x="-25%" y="-25%" width="150%" height="170%">'
    '<feDropShadow dx="2" dy="4" stdDeviation="3.5" flood-color="#000" flood-opacity="0.26"/></filter>'
    '<marker id="tri" markerWidth="22" markerHeight="16" refX="15" refY="7.5" orient="auto">'
    '<path d="M2,2 Q10,0.5 15.5,7.5 Q9.5,14 2.5,12.5 Q4,7.5 2,2 Z" fill="#fbfbf5" stroke="#2b2b2b" '
    'stroke-width="1.5" stroke-linejoin="round" stroke-linecap="round"/></marker>'
    '<marker id="use" markerWidth="13" markerHeight="13" refX="9" refY="5.5" orient="auto">'
    '<path d="M1.5,1.5 L10,5.5 L1.5,9.5" fill="none" stroke="#2b2b2b" stroke-width="1.7" '
    'stroke-linecap="round" stroke-linejoin="round"/></marker>'
    '</defs>'
)


def _interface_box(cx, top, role, page, rot, font, is_list):
    """Green «interface» sticky (the base role, or a collaborator role), linked to
    its API page. A list collaborator gets a second card offset behind it."""
    left = cx - _IF_W / 2
    out = [f'<a href="{BASE_URL}/{page}/" target="_top" class="node">',
           f'<g transform="rotate({rot} {cx} {top + _IF_H / 2})">']
    if is_list:
        out.append(f'<g filter="url(#sh)"><rect class="card" x="{left + 8}" y="{top + 8}" width="{_IF_W}" height="{_IF_H}" '
                   f'fill="{_GREEN[0]}" stroke="{_GREEN[1]}" stroke-width="1.4"/></g>')
    out.append(f'<g filter="url(#sh)"><rect class="card" x="{left}" y="{top}" width="{_IF_W}" height="{_IF_H}" '
               f'fill="{_GREEN[0]}" stroke="{_GREEN[1]}" stroke-width="1.4"/></g>')
    out.append(f'<text x="{cx}" y="{top + 23}" text-anchor="middle" font-size="11.5" font-style="italic" '
               f'font-family="{_UI}" fill="{_GREEN[2]}">«interface»</text>')
    out.append(f'<text x="{cx}" y="{top + 45}" text-anchor="middle" font-size="{font}" font-weight="bold" '
               f'fill="{_GREEN[2]}">SolidSyslog{role}</text>')
    out.append("</g></a>")
    return "".join(out)


def _subject_box(cx, top, name, package, is_null, font, page=None):
    """Blue sticky for the class the page is about — the one concrete node, so it
    reads as the focus. Grey for a Null object. Links to @p page when given (as a
    sidecar node on another page's diagram); unlinked when it is the page itself."""
    fill, stroke, text = _GREY if is_null else _PERI
    pkg_color = _PKG_NULL if is_null else _PKG
    pkg = package + (" · no-op" if is_null else "")
    left = cx - _SUB_W / 2
    out = [f'<g transform="rotate(-1 {cx} {top + _SUB_H / 2})">',
           f'<g filter="url(#sh)"><rect class="card" x="{left}" y="{top}" width="{_SUB_W}" height="{_SUB_H}" fill="{fill}" '
           f'stroke="{stroke}" stroke-width="1.8"/></g>',
           f'<text x="{cx}" y="{top + 28}" text-anchor="middle" font-size="{font}" font-weight="bold" '
           f'fill="{text}">{name}</text>',
           f'<text x="{cx}" y="{top + 47}" text-anchor="middle" font-size="{max(8.0, font - 2)}" font-style="italic" '
           f'font-family="{_UI}" fill="{pkg_color}">{pkg}</text>',
           "</g>"]
    body = "".join(out)
    if page:
        return f'<a href="{BASE_URL}/{page}/" target="_top" class="node">{body}</a>'
    return body


def _multiplicity(is_optional, is_list):
    """UML multiplicity from the two structural facts. Lower bound 0 iff the slot
    is @optional, else 1 (the header's "required" contract); upper bound * iff a
    list. Exactly-one (1..1) is conventionally left unlabelled."""
    lower = "0" if is_optional else "1"
    upper = "*" if is_list else "1"
    return None if (lower, upper) == ("1", "1") else lower + ".." + upper


def render_reverse(entry):
    name = entry["name"]
    base_role = entry["base_role"]
    collabs = entry["collaborators"]
    n = len(collabs)

    row_w = n * _IF_W + (n - 1) * _COL_GAP if n else 0
    width = round(_MARGIN * 2 + max(row_w, _SUB_W, _IF_W))
    cx = width / 2

    base_y = _MARGIN
    sub_y = base_y + _IF_H + _V_REALISES
    col_y = sub_y + _SUB_H + _V_USES
    height = round((col_y + _IF_H if n else sub_y + _SUB_H) + _MARGIN)

    col_cx = [cx - row_w / 2 + _IF_W / 2 + i * (_IF_W + _COL_GAP) for i in range(n)]

    base_font = _fit(len("SolidSyslog" + base_role), _IF_W - 20, 13.5)
    sub_font = _fit(len(name), _SUB_W - 14, 13.0)
    col_font = base_font
    if n:
        longest = max(len("SolidSyslog" + c["role"]) for c in collabs)
        col_font = _fit(longest, _IF_W - 20, 13.5)

    svg_id = "ssrev-" + name.lower()
    parts = ['<div class="ss-rel">',
             f'<svg id="{svg_id}" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" '
             f'font-family="{_HAND}" style="width:100%;max-width:{min(width, 760)}px;display:block;margin:0.4rem auto 1.4rem;">',
             _REVERSE_DEFS,
             f'<style>#{svg_id} .node{{cursor:pointer}}#{svg_id} .card{{transition:stroke-width .1s ease}}'
             f'#{svg_id} .node:hover .card{{stroke-width:3.2}}</style>']

    # connectors, drawn behind the stickies
    edges = ['<g fill="none" stroke="#2b2b2b" filter="url(#rough)">',
             f'<path d="M{cx},{sub_y} L{cx},{base_y + _IF_H + 1}" stroke-width="2" stroke-dasharray="6 5" '
             'marker-end="url(#tri)"/>']
    for ccx in col_cx:
        edges.append(f'<path d="M{cx},{sub_y + _SUB_H} L{ccx},{col_y}" stroke-width="2.2" '
                     'marker-end="url(#use)"/>')
    edges.append("</g>")
    parts += edges

    parts.append(_interface_box(cx, base_y, base_role, entry["base_page"], -1, base_font, False))
    parts.append(_subject_box(cx, sub_y, name, entry["package"], entry["is_null"], sub_font))
    _place_collaborators(parts, col_cx, collabs, col_y, col_font)

    parts.append("</svg></div>")
    return "".join(parts)


def _place_collaborators(parts, col_cx, collabs, col_y, col_font):
    """Draw the collaborator interface boxes and their UML multiplicity labels
    (1..1 left unlabelled). Shared by the reverse and facade views."""
    for idx, (ccx, collab) in enumerate(zip(col_cx, collabs, strict=True)):
        parts.append(_interface_box(ccx, col_y, collab["role"], collab["base_page"],
                                    _ROT[idx % len(_ROT)], col_font, collab["is_list"]))
        mult = _multiplicity(collab["is_optional"], collab["is_list"])
        if mult:
            parts.append(f'<text x="{ccx + 14}" y="{col_y - 7}" font-size="11" font-style="italic" '
                         f'font-family="{_UI}" fill="#3a4a80">{mult}</text>')


# ---------------------------------------------------------------------------
# Facade view: the composition root (SolidSyslog) and the roles its config wires
# it to. Uses-only — no realises edge, because the facade implements no role.
# ---------------------------------------------------------------------------

_FAC_W, _FAC_H = 176, 64


def render_facade_class(facade):
    """The SolidSyslog page: the class with direct arrows to the interfaces it
    uses, and its Config as a linked sidecar (dashed — built from, not retained)."""
    _fac_page, cfg_page, cfg_name, collabs, fac_font, cfg_font, col_font = _facade_common(facade)
    n = len(collabs)
    pair_gap = 244
    row_w = n * _IF_W + (n - 1) * _COL_GAP if n else 0
    cx_main = _MARGIN + row_w / 2
    cx_cfg = cx_main + pair_gap
    width = round(max(_MARGIN * 2 + row_w, cx_cfg + _FAC_W / 2 + _MARGIN))
    top_y = _MARGIN
    col_y = top_y + _FAC_H + 46
    height = round(col_y + _IF_H + _MARGIN)
    col_cx = [cx_main - row_w / 2 + _IF_W / 2 + i * (_IF_W + _COL_GAP) for i in range(n)]

    parts = _facade_open("ssfacc-" + facade["name"].lower(), width, height)
    parts.append('<g fill="none" stroke="#2b2b2b" filter="url(#rough)">')
    parts.append(f'<path d="M{cx_main + _SUB_W / 2},{top_y + _SUB_H / 2} L{cx_cfg - _FAC_W / 2},{top_y + _SUB_H / 2}" stroke-width="2" stroke-dasharray="6 5" '
                 'marker-end="url(#use)"/>')
    for ccx in col_cx:
        parts.append(f'<path d="M{cx_main},{top_y + _SUB_H} L{ccx},{col_y}" stroke-width="2.2" '
                     'marker-end="url(#use)"/>')
    parts.append("</g>")
    parts.append(f'<text x="{(cx_main + cx_cfg) / 2}" y="{top_y + _SUB_H / 2 - 13}" text-anchor="middle" font-size="10" font-style="italic" '
                 f'font-family="{_UI}" fill="#666">built from</text>')
    parts.append(_subject_box(cx_main, top_y, facade["name"], "Core", False, fac_font))
    parts.append(_pod_box(cx_cfg, top_y, cfg_name, cfg_font, 1, page=cfg_page))
    _place_collaborators(parts, col_cx, collabs, col_y, col_font)
    parts.append("</svg></div>")
    return "".join(parts)


# The config POD, shown as a distinct linked node beside the SolidSyslog class.
_POD = ("#dfe3ea", "#9aa6b8", "#33404f")


def _pod_box(cx, top, name, font, rot, page=None):
    """Pale-blue «struct» sticky for a POD (the config). Links to @p page when
    given (a sidecar on the class page); unlinked when it is the page itself."""
    left = cx - _FAC_W / 2
    body = "".join([
        f'<g transform="rotate({rot} {cx} {top + _FAC_H / 2})">',
        f'<g filter="url(#sh)"><rect class="card" x="{left}" y="{top}" width="{_FAC_W}" height="{_FAC_H}" '
        f'fill="{_POD[0]}" stroke="{_POD[1]}" stroke-width="1.6"/></g>',
        f'<text x="{cx}" y="{top + 22}" text-anchor="middle" font-size="11.5" font-style="italic" '
        f'font-family="{_UI}" fill="{_POD[2]}">«struct»</text>',
        f'<text x="{cx}" y="{top + 45}" text-anchor="middle" font-size="{font}" font-weight="bold" '
        f'fill="{_POD[2]}">{name}</text>',
        "</g>"])
    if page:
        return f'<a href="{BASE_URL}/{page}/" target="_top" class="node">{body}</a>'
    return body


def _facade_open(svg_id, width, height):
    return ['<div class="ss-rel">',
            f'<svg id="{svg_id}" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" '
            f'font-family="{_HAND}" style="width:100%;max-width:{min(width, 860)}px;display:block;margin:0.4rem auto 1.4rem;">',
            _REVERSE_DEFS,
            f'<style>#{svg_id} .node{{cursor:pointer}}#{svg_id} .card{{transition:stroke-width .1s ease}}'
            f'#{svg_id} .node:hover .card{{stroke-width:3.2}}</style>']


def _facade_common(facade):
    fac_page, cfg_page = facade["pages"][0], facade["pages"][1]
    cfg_name = cfg_page[len("struct"):] if cfg_page.startswith("struct") else cfg_page[:-3]
    collabs = facade["collaborators"]
    fac_font = _fit(len(facade["name"]), _FAC_W - 16, 14.0)
    cfg_font = _fit(len(cfg_name), _FAC_W - 16, 14.0)
    col_font = _fit(max(len("SolidSyslog" + c["role"]) for c in collabs), _IF_W - 20, 13.5) if collabs else 13.0
    return fac_page, cfg_page, cfg_name, collabs, fac_font, cfg_font, col_font


def render_facade_stacked(facade):
    """The SolidSyslogConfig page: the config POD with its interface members below,
    and the SolidSyslog class it builds as a linked node above (dashed — built from)."""
    fac_page, _cfg_page, cfg_name, collabs, fac_font, cfg_font, col_font = _facade_common(facade)
    n = len(collabs)
    row_w = n * _IF_W + (n - 1) * _COL_GAP if n else 0
    width = round(_MARGIN * 2 + max(row_w, _FAC_W))
    cx = width / 2
    fac_y = _MARGIN
    cfg_y = fac_y + _SUB_H + 40
    col_y = cfg_y + _FAC_H + 46
    height = round(col_y + _IF_H + _MARGIN)
    col_cx = [cx - row_w / 2 + _IF_W / 2 + i * (_IF_W + _COL_GAP) for i in range(n)]

    parts = _facade_open("ssfacs-" + facade["name"].lower(), width, height)
    parts.append('<g fill="none" stroke="#2b2b2b" filter="url(#rough)">')
    parts.append(f'<path d="M{cx},{fac_y + _SUB_H} L{cx},{cfg_y}" stroke-width="2" stroke-dasharray="6 5" '
                 'marker-end="url(#use)"/>')
    for ccx in col_cx:
        parts.append(f'<path d="M{cx},{cfg_y + _FAC_H} L{ccx},{col_y}" stroke-width="2.2" '
                     'marker-end="url(#use)"/>')
    parts.append("</g>")
    parts.append(f'<text x="{cx + 14}" y="{(fac_y + _SUB_H + cfg_y) / 2 + 3}" font-size="10" font-style="italic" '
                 f'font-family="{_UI}" fill="#666">built from</text>')
    parts.append(_subject_box(cx, fac_y, facade["name"], "Core", False, fac_font, page=fac_page))
    parts.append(_pod_box(cx, cfg_y, cfg_name, cfg_font, 1))
    _place_collaborators(parts, col_cx, collabs, col_y, col_font)
    parts.append("</svg></div>")
    return "".join(parts)


# ---------------------------------------------------------------------------
# Combined role view: an interface in its ecosystem — its clients above (uses,
# solid) and its implementers below (realises, hollow triangle). Injected on the
# role page, completing the navigation graph in both directions.
# ---------------------------------------------------------------------------

_CONS_GAP = 46


def render_role_combined(role, info, consumers):
    impls = info["implementers"]
    n = len(impls)
    two_rows = n >= 5
    top_n = (n + 1) // 2 if two_rows else n
    top, bottom = impls[:top_n], impls[top_n:]

    c0 = _MARGIN + _STICKY_W / 2
    top_cx = [c0 + i * _PITCH for i in range(len(top))]
    bot_cx = [c0 + _PITCH / 2 + j * _PITCH for j in range(len(bottom))]
    all_cx = top_cx + bot_cx
    span_min, span_max = min(all_cx), max(all_cx)
    base_cx = (span_min + span_max) / 2

    m = len(consumers)
    cons_pitch = _SUB_W + _GAP
    cons_row_w = m * _SUB_W + (m - 1) * _GAP if m else 0
    cons_cx = [base_cx - cons_row_w / 2 + _SUB_W / 2 + i * cons_pitch for i in range(m)]

    # A consumer row wider than the implementer span, centred on base_cx, can spill
    # left of the margin (as can the wider interface box for a lone implementer).
    # Shift every x right so the leftmost box sits at the margin, and take width from
    # the true left/right extents rather than the rightmost box alone.
    boxes = [(cx, _STICKY_W) for cx in all_cx] + [(base_cx, _BASE_W)] + [(cx, _SUB_W) for cx in cons_cx]
    shift = _MARGIN - min(cx - w / 2 for cx, w in boxes)
    top_cx = [cx + shift for cx in top_cx]
    bot_cx = [cx + shift for cx in bot_cx]
    cons_cx = [cx + shift for cx in cons_cx]
    base_cx += shift
    span_min += shift
    span_max += shift
    width = round(max(cx + w / 2 for cx, w in boxes) + shift + _MARGIN)

    cons_y = _MARGIN
    base_y = (cons_y + _SUB_H + _CONS_GAP) if m else _MARGIN
    base_bottom = base_y + _BASE_H
    bus_y = base_bottom + _BUS_GAP
    row1_y = bus_y + _ROW_GAP
    row2_y = row1_y + _STICKY_H + _ROW_GAP

    wraps = _WRAPS.get(role, set())
    name_font = _fit(max(len(i["name"]) for i in impls), _STICKY_W - 16, 12.0)
    pkg_font = max(8.0, name_font - 2.0)
    base_font = _fit(len("SolidSyslog" + role), _BASE_W - 24, 15.5)
    cons_font = _fit(max(len(c["name"]) for c in consumers), _SUB_W - 14, 12.5) if m else 12.0

    last_bottom = (row2_y if bottom else row1_y) + _STICKY_H
    height = round(last_bottom + (20 if wraps else 8) + _MARGIN)

    parts = _facade_open("ssuses-" + role.lower(), width, height)

    # realises tree (implementers below the interface)
    tree = ['<g fill="none" stroke="#2b2b2b" stroke-width="2" stroke-dasharray="6 5" filter="url(#rough)">',
            f'<path d="M{base_cx},{bus_y} L{base_cx},{base_bottom + 1}" marker-end="url(#tri)"/>',
            f'<path d="M{span_min},{bus_y} H{span_max}"/>']
    for cx in top_cx:
        tree.append(f'<path d="M{cx},{bus_y} V{row1_y}"/>')
    for cx in bot_cx:
        tree.append(f'<path d="M{cx},{bus_y} V{row2_y}"/>')
    tree.append("</g>")
    parts += tree

    # uses edges (clients above -> into the interface top)
    parts.append('<g fill="none" stroke="#2b2b2b" filter="url(#rough)">')
    for ccx in cons_cx:
        parts.append(f'<path d="M{ccx},{cons_y + _SUB_H} L{base_cx},{base_y}" stroke-width="2.2" '
                     'marker-end="url(#use)"/>')
    parts.append("</g>")

    # interface (base)
    parts.append('<a href="{}/{}/" target="_top" class="node"><g transform="rotate(-1 {} {})">'.format(
        BASE_URL, info["link_page"], base_cx, base_y + _BASE_H / 2))
    parts.append(f'<g filter="url(#sh)"><rect class="card" x="{base_cx - _BASE_W / 2}" y="{base_y}" width="{_BASE_W}" height="{_BASE_H}" '
                 f'fill="{_GREEN[0]}" stroke="{_GREEN[1]}" stroke-width="1.4"/></g>')
    parts.append(f'<text x="{base_cx}" y="{base_y + 26}" text-anchor="middle" font-size="12" font-style="italic" '
                 f'font-family="{_UI}" fill="{_GREEN[2]}">«interface»</text>')
    parts.append(f'<text x="{base_cx}" y="{base_y + 50}" text-anchor="middle" font-size="{base_font}" font-weight="bold" '
                 f'fill="{_GREEN[2]}">SolidSyslog{role}</text>')
    parts.append("</g></a>")

    for idx, (cx, impl) in enumerate(zip(top_cx, top, strict=True)):
        parts.append(_sticky(cx, row1_y, impl, idx, name_font, pkg_font, role, wraps))
    for j, (cx, impl) in enumerate(zip(bot_cx, bottom, strict=True)):
        parts.append(_sticky(cx, row2_y, impl, top_n + j, name_font, pkg_font, role, wraps))

    for ccx, consumer in zip(cons_cx, consumers, strict=True):
        parts.append(_subject_box(ccx, cons_y, consumer["name"], consumer["package"],
                                  False, cons_font, page=consumer["page"]))

    parts.append("</svg></div>")
    return "".join(parts)
