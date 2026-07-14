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

BASE_URL = "/solid-syslog/api"

# fill, stroke, text
_GREEN = ("#bfe3c0", "#6fae74", "#21401f")
_PERI = ("#cdd4f5", "#8f9bd6", "#15324f")
_GREY = ("#e4e4e2", "#b0b0a8", "#45453d")
_PINK = ("#f6c6c6", "#cf8a8a", "#4a2c2c")
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
           '<g transform="rotate({} {} {})">'.format(rot, cx, cy)]
    if impl["name"] in wraps:
        ty = y + _STICKY_H - 6
        out.append('<g filter="url(#sh)"><rect x="{}" y="{}" width="68" height="22" rx="2" '
                   'fill="{}" stroke="{}" stroke-width="1.2"/></g>'.format(cx - 34, ty, _PINK[0], _PINK[1]))
        out.append('<text x="{}" y="{}" text-anchor="middle" font-size="8.5" font-family="{}" '
                   'fill="{}">wraps a {}</text>'.format(cx, ty + 15, _UI, _PINK[2], role))
    out.append('<g filter="url(#sh)"><rect class="card" x="{}" y="{}" width="{}" height="{}" '
               'fill="{}" stroke="{}" stroke-width="1.4"/></g>'.format(left, y, _STICKY_W, _STICKY_H, fill, stroke))
    out.append('<text x="{}" y="{}" text-anchor="middle" font-size="{}" font-family="{}" '
               'font-weight="bold" fill="{}">{}</text>'.format(cx, y + 26, name_font, _HAND, text, impl["name"]))
    out.append('<text x="{}" y="{}" text-anchor="middle" font-size="{}" font-style="italic" '
               'font-family="{}" fill="{}">{}</text>'.format(cx, y + 45, pkg_font, _UI, pkg_color, pkg))
    out.append("</g></a>")
    return "".join(out)


def render_svg(role, info):
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

    base_bottom = _BASE_Y + _BASE_H
    bus_y = base_bottom + _BUS_GAP
    row1_y = bus_y + _ROW_GAP
    row2_y = row1_y + _STICKY_H + _ROW_GAP

    wraps = _WRAPS.get(role, set())
    max_name = max(len(i["name"]) for i in impls)
    name_font = _fit(max_name, _STICKY_W - 16, 12.0)
    pkg_font = max(8.0, name_font - 2.0)
    base_font = _fit(len("SolidSyslog" + role), _BASE_W - 24, 15.5)

    last_bottom = (row2_y if bottom else row1_y) + _STICKY_H
    height = round(last_bottom + (20 if wraps else 8) + _MARGIN)
    width = round(max([cx + _STICKY_W / 2 for cx in all_cx] + [base_cx + _BASE_W / 2]) + _MARGIN)
    svg_id = "ssrel-" + role.lower()

    parts = ['<div class="ss-rel">',
             '<svg id="{}" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {} {}" '
             'font-family="{}" style="width:100%;max-width:{}px;display:block;margin:0.4rem auto 1.4rem;">'.format(
                 svg_id, width, height, _HAND, min(width, 880)),
             '<defs>'
             '<filter id="rough" x="-6%" y="-6%" width="112%" height="112%">'
             '<feTurbulence type="fractalNoise" baseFrequency="0.012" numOctaves="2" seed="7" result="n"/>'
             '<feDisplacementMap in="SourceGraphic" in2="n" scale="3"/></filter>'
             '<filter id="sh" x="-25%" y="-25%" width="150%" height="170%">'
             '<feDropShadow dx="2" dy="4" stdDeviation="3.5" flood-color="#000" flood-opacity="0.26"/></filter>'
             '<marker id="tri" markerWidth="22" markerHeight="16" refX="15" refY="7.5" orient="auto">'
             '<path d="M2,2 Q10,0.5 15.5,7.5 Q9.5,14 2.5,12.5 Q4,7.5 2,2 Z" fill="#fbfbf5" stroke="#2b2b2b" '
             'stroke-width="1.5" stroke-linejoin="round" stroke-linecap="round"/></marker></defs>',
             '<style>#{0} .node{{cursor:pointer}}#{0} .card{{transition:stroke-width .1s ease}}'
             '#{0} .node:hover .card{{stroke-width:3.2}}</style>'.format(svg_id)]

    # realises generalisation tree (behind the stickies)
    tree = ['<g fill="none" stroke="#2b2b2b" stroke-width="2" stroke-dasharray="6 5" filter="url(#rough)">',
            '<path d="M{0},{1} L{0},{2}" marker-end="url(#tri)"/>'.format(base_cx, bus_y, base_bottom + 1),
            '<path d="M{},{} H{}"/>'.format(span_min, bus_y, span_max)]
    for cx in top_cx:
        tree.append('<path d="M{},{} V{}"/>'.format(cx, bus_y, row1_y))
    for cx in bot_cx:
        tree.append('<path d="M{},{} V{}"/>'.format(cx, bus_y, row2_y))
    tree.append("</g>")
    parts += tree

    # interface (base)
    parts.append('<a href="{}/{}/" target="_top" class="node"><g transform="rotate(-1 {} {})">'.format(
        BASE_URL, info["struct_page"], base_cx, _BASE_Y + _BASE_H / 2))
    parts.append('<g filter="url(#sh)"><rect class="card" x="{}" y="{}" width="{}" height="{}" '
                 'fill="{}" stroke="{}" stroke-width="1.4"/></g>'.format(
                     base_cx - _BASE_W / 2, _BASE_Y, _BASE_W, _BASE_H, _GREEN[0], _GREEN[1]))
    parts.append('<text x="{}" y="{}" text-anchor="middle" font-size="12" font-style="italic" '
                 'font-family="{}" fill="{}">«interface»</text>'.format(base_cx, _BASE_Y + 26, _UI, _GREEN[2]))
    parts.append('<text x="{}" y="{}" text-anchor="middle" font-size="{}" font-weight="bold" '
                 'fill="{}">SolidSyslog{}</text>'.format(base_cx, _BASE_Y + 50, base_font, _GREEN[2], role))
    parts.append("</g></a>")

    for idx, (cx, impl) in enumerate(zip(top_cx, top)):
        parts.append(_sticky(cx, row1_y, impl, idx, name_font, pkg_font, role, wraps))
    for j, (cx, impl) in enumerate(zip(bot_cx, bottom)):
        parts.append(_sticky(cx, row2_y, impl, top_n + j, name_font, pkg_font, role, wraps))

    parts.append("</svg></div>")
    return "".join(parts)
