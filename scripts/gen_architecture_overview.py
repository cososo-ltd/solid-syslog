"""Generate docs/assets/postit/architecture-overview.svg.

The Core rectangle is a boundary. Everything inside is portable C. A touch
point is literal: a line crossing out of Core, through a thin platform adapter,
into something you supply. Data flow is a separate set of diagrams.

Palette is the post-it kit's (docs/assets/postit/README.md): yellow facade,
green role, blue adapter, pink third party. Core is the container, not a colour.

Links: the facade -> the SolidSyslog.h header page; every other green role ->
its structSolidSyslog<Role> page; anywhere on the platform-adapters band -> the
platforms index.

Run: python scripts/gen_architecture_overview.py  (writes the SVG in place).
"""

import os

GREEN, GREEN_E = "#bfe3c0", "#8fbf92"
YELLOW, YELLOW_E = "#ffe7a3", "#d8b94e"    # kit's facade colour
BLUE, BLUE_E = "#cdd4f5", "#9aa6e0"
RED, RED_E = "#f6c6c6", "#dba3a3"
CORE_FILL = "#eef6ee"
BLUE_BG = "#e7ebfa"
TXT_G, TXT_R, TXT_Y = "#21401f", "#4a2c2c", "#5a481a"

PLATFORMS = "../../platforms/"
FACADE_LINK = "../../api/SolidSyslog_8h/"

GROUPS = [
    ("Sender", [
        ("Resolver", ["name resolution"]),
        ("Datagram", ["UDP sockets"]),
        ("Stream", ["TCP sockets", "TLS library"]),
    ]),
    ("Store", [
        ("File", ["filesystem"]),
        ("SecurityPolicy", ["crypto library"]),
    ]),
    ("Buffer", [("Mutex", ["OS mutex"])]),
    ("StructuredData", [("AtomicCounter", ["atomics"])]),
]

BOX_W, BOX_H = 126, 48
ADP_W = 100
SLOT = BOX_W + 16            # tight within-group spacing
GRP_GAP = 48                 # clearly bigger between families
GUTTER = 96
X0 = GUTTER + 28

Y_FACADE = 56
Y_CONFIG = 148
Y_TOUCH = 240               # bottom row -- sits on the Core boundary
Y_BLUE = 336
Y_RED = 424
BLUE_H = 40

out = []


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def fs(t):
    return 11.5 if len(t) > 13 else (12.5 if len(t) > 10 else 14)


def box(cx, y, label, fill, edge, link=None, cls="node", *, bold=False, w=BOX_W, h=BOX_H):
    xx = cx - w / 2
    weight = ' font-weight="700"' if bold else ' font-weight="600"'
    rect = (f'<rect x="{xx:.0f}" y="{y:.0f}" width="{w:.0f}" height="{h}" rx="7" '
            f'fill="{fill}" stroke="{edge}" stroke-width="1.5"/>')
    tcol = TXT_R if fill == RED else (TXT_Y if fill == YELLOW else TXT_G)
    txt = (f'<text x="{cx:.0f}" y="{y + h/2:.0f}" text-anchor="middle" dominant-baseline="middle" '
           f'font-size="{fs(label)}"{weight} fill="{tcol}">{esc(label)}</text>')
    if link:
        out.append(f'  <a href="{link}" target="_top" class="{cls}">\n    {rect}\n    {txt}\n  </a>')
    else:
        out.append(f"  {rect}\n  {txt}")


# ---- layout ---------------------------------------------------------------
touch, config = [], []
cursor = X0
for cfg, members in GROUPS:
    g_left = cursor
    for name, exts in members:
        span = len(exts)
        slot_centres = [cursor + SLOT / 2 + i * SLOT for i in range(span)]
        touch.append({"name": name, "cx": cursor + span * SLOT / 2,
                      "slots": slot_centres, "exts": exts})
        cursor += span * SLOT
    config.append((cfg, (g_left + cursor) / 2))
    cursor += GRP_GAP

RIGHT = cursor - GRP_GAP
W = RIGHT + 40
H = Y_RED + BOX_H + 34

core_l, core_r = GUTTER + 8, RIGHT + 8
core_top = Y_FACADE - 24
core_bot = Y_TOUCH + BOX_H + 16
band_top = Y_BLUE - 28
band_h = BLUE_H + 42

# ---- Core boundary --------------------------------------------------------
out.append(f'  <rect x="{core_l}" y="{core_top}" width="{core_r - core_l}" height="{core_bot - core_top}" '
           f'rx="18" fill="{CORE_FILL}" stroke="{GREEN_E}" stroke-width="2.2"/>')
out.append(f'  <text x="{core_l + 18}" y="{core_top + 26}" font-size="17" font-weight="700" fill="#3a7a40">Core</text>')

# ---- platform-adapter band (whole band links to the platforms page) ------
out.append(f'  <a href="{PLATFORMS}" target="_top" class="plat">')
out.append(f'    <rect x="{core_l}" y="{band_top}" width="{core_r - core_l}" height="{band_h}" '
           f'rx="14" fill="{BLUE_BG}" stroke="{BLUE_E}" stroke-width="1.6"/>')
out.append(f'    <text x="{core_l + 18}" y="{band_top + 24}" font-size="14.5" font-weight="700" fill="#5a67c0">Platform adapters</text>')
out.append('  </a>')

# ---- facade + config roles (no wires -- position carries the hierarchy) --
facade_cx = (core_l + core_r) / 2
box(facade_cx, Y_FACADE, "SolidSyslog", YELLOW, YELLOW_E, link=FACADE_LINK, bold=True, w=184, h=50)
for cfg, ccx in config:
    box(ccx, Y_CONFIG, cfg, GREEN, GREEN_E, link=f'../../api/structSolidSyslog{cfg}/')

# ---- touch row: roles crossing the Core boundary to your system ----------
for t in touch:
    box(t["cx"], Y_TOUCH, t["name"], GREEN, GREEN_E, link=f'../../api/structSolidSyslog{t["name"]}/')
    box_bottom = Y_TOUCH + BOX_H
    mid = (box_bottom + band_top) / 2
    for cx2, ext in zip(t["slots"], t["exts"], strict=True):
        # fan out of the box, cross the boundary, into the adapter
        out.append(f'  <path class="wire" d="M {t["cx"]:.0f} {box_bottom} V {mid:.0f} H {cx2:.0f} V {Y_BLUE:.0f}"/>')
        out.append(f'  <a href="{PLATFORMS}" target="_top" class="plat">'
                   f'<rect x="{cx2 - ADP_W/2:.0f}" y="{Y_BLUE}" width="{ADP_W}" height="{BLUE_H}" rx="6" '
                   f'fill="{BLUE}" stroke="{BLUE_E}" stroke-width="1.5"/></a>')
        out.append(f'  <line x1="{cx2:.0f}" y1="{Y_BLUE + BLUE_H}" x2="{cx2:.0f}" y2="{Y_RED}" class="wire"/>')
        box(cx2, Y_RED, ext, RED, RED_E, w=ADP_W + 20)

# ---- dependency arrows, left gutter: direction, not connection -----------
gx = GUTTER / 2
out.append(f'''  <g stroke="#6a74c4" stroke-width="8" fill="none" stroke-linecap="round">
    <path d="M {gx} {Y_BLUE - 8} V {core_bot - 2}" marker-end="url(#big)"/>
    <path d="M {gx} {Y_BLUE + BLUE_H + 8} V {Y_RED + 2}" marker-end="url(#big)"/>
  </g>''')

svg = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W:.0f} {H}" font-family="'Segoe UI', sans-serif">
  <desc>SolidSyslog architecture overview. The Core rectangle is a boundary; everything
  inside is portable C. You wire four roles on the SolidSyslog facade: Sender, Store,
  Buffer and Structured Data. Beneath them, the roles that cross the Core boundary to
  reach your system -- name resolution, UDP sockets, TCP sockets and a TLS library
  (Sender); the filesystem and a crypto library (Store); an OS mutex (Buffer); atomics
  (Structured Data) -- each through a thin platform adapter you supply. The adapter layer
  depends inward on Core and outward on your system; Core depends on nothing outside
  itself.</desc>
  <defs>
    <marker id="big" markerWidth="7" markerHeight="6" refX="3.4" refY="3" orient="auto">
      <path d="M0 0 L7 3 L0 6 Z" fill="#6a74c4"/></marker>
  </defs>
  <style>
    .wire {{ fill:none; stroke:#8fbf92; stroke-width:1.7; }}
    .node, .plat {{ cursor:pointer; }}
    .node:hover rect {{ stroke:#3a7a40; stroke-width:3; }}
    .plat:hover rect {{ stroke:#3a4590; stroke-width:2.6; }}
  </style>
  <rect width="{W:.0f}" height="{H}" fill="#fbfbf5"/>

{chr(10).join(out)}
</svg>
'''

here = os.path.dirname(os.path.abspath(__file__))
path = os.path.normpath(os.path.join(here, "..", "docs", "assets", "postit", "architecture-overview.svg"))
with open(path, "w", encoding="utf-8") as f:
    f.write(svg)
print(f"wrote {path}  ({W:.0f}x{H})")
