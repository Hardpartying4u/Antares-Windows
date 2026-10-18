#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Compare an original sprite with re-rendered variants: a detail sheet and a readability sheet.

    python tools/sprite-render/variant_sheet.py --sprite uns/cruiser \
        --variant A=out/sprite-render/variant-A --variant B=... --out out/sprite-render

Needs Pillow. Writes <sprite>-variants-detail.png (original vs variants at 2x and 4x; the original
is enlarged with nearest-neighbour, since it has no higher-resolution art) and
<sprite>-variants-readability.png (each at its real 1x pixel size and at 1:2 zoom, as the game
shows them at UI scale 1, plus the same enlarged 4x for inspection, plus a team-tinted version to
check the overlay). Images show image.png as players see it: sides are untinted unless a level
assigns them a colour.
"""

import argparse
import os
import re

import numpy as np
from PIL import Image, ImageDraw

FRAME_RE = re.compile(
    r"left:\s*(-?\d+),\s*top:\s*(-?\d+),\s*right:\s*(-?\d+),\s*bottom:\s*(-?\d+),"
    r"\s*cx:\s*(-?\d+),\s*cy:\s*(-?\d+)")
BG = (14, 14, 18, 255)
LABEL = (230, 210, 90, 255)
TEXT = (200, 200, 205, 255)
GREEN = (0, 256, 0)  # kDiffuse for Hue::GREEN (src/drawing/color.cpp)


def load(path):
    return Image.open(path).convert("RGBA")


def tinted(image, overlay, diffuse=GREEN):
    """The game's team tint: overlay brightness times the hue, blended over the image."""
    img = np.asarray(image).astype(float)
    ov = np.asarray(overlay).astype(float)
    frac = ov[..., 3:4] / 255
    tint = np.array(diffuse, float)[None, None, :] * ov[..., 0:1] / 256
    out = img.copy()
    out[..., :3] = tint * frac + img[..., :3] * (1 - frac)
    return Image.fromarray(out.clip(0, 255).astype(np.uint8), "RGBA")


def crop(sheet, frame, s):
    l, t, r, b = frame[:4]
    return sheet.crop((l * s, t * s, r * s, b * s))


def half(img):
    """1:2 zoom as the game draws it: nearest-neighbour sampling at half size."""
    return img.resize((max(1, img.width // 2), max(1, img.height // 2)), Image.NEAREST)


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("--sprite", required=True)
    p.add_argument("--data", default="data")
    p.add_argument("--variant", action="append", required=True, help="LABEL=render output root")
    p.add_argument("--frames", default="0,3,6,9,12", help="frames to show (default: 0,3,6,9,12)")
    p.add_argument("--out", default="out/sprite-render")
    a = p.parse_args()

    with open(os.path.join(a.data, "sprites", a.sprite + ".pn"), encoding="utf-8") as f:
        frames = [tuple(map(int, m)) for m in FRAME_RE.findall(f.read())]
    show = [int(x) for x in a.frames.split(",")]
    orig_dir = os.path.join(a.data, "sprites", a.sprite)
    rows = [("original", {1: load(f"{orig_dir}/image.png")}, load(f"{orig_dir}/overlay.png"))]
    for spec in a.variant:
        label, root = spec.split("=", 1)
        d = os.path.join(root, "sprites", a.sprite)
        rows.append((label, {s: load(f"{d}/image{'' if s == 1 else f'@{s}x'}.png") for s in (1, 2, 4)},
                     load(f"{d}/overlay.png")))
    fw = max(f[2] - f[0] for f in frames)
    fh = max(f[3] - f[1] for f in frames)
    name = a.sprite.replace("/", "-")

    # Detail sheet: for each scale (2x, 4x) a block of rows x frames.
    label_w, gap = 120, 10
    blocks = []
    for s in (2, 4):
        cw, ch = fw * s + gap, fh * s + gap
        block = Image.new("RGBA", (label_w + cw * len(show), 26 + ch * len(rows)), BG)
        d = ImageDraw.Draw(block)
        d.text((6, 6), f"at {s}x (original enlarged, variants from their @{s}x sheets)", fill=LABEL)
        for r, (label, sheets, _) in enumerate(rows):
            y = 26 + r * ch
            d.text((6, y + ch // 2 - 6), label, fill=TEXT)
            for k, i in enumerate(show):
                if s in sheets:
                    img = crop(sheets[s], frames[i], s)
                else:
                    img = crop(sheets[1], frames[i], 1).resize(
                        ((frames[i][2] - frames[i][0]) * s, (frames[i][3] - frames[i][1]) * s),
                        Image.NEAREST)
                block.alpha_composite(img, (label_w + k * cw, y))
        blocks.append(block)
    detail = Image.new("RGBA", (max(b.width for b in blocks), sum(b.height for b in blocks) + 10), BG)
    y = 0
    for b in blocks:
        detail.alpha_composite(b, (0, y))
        y += b.height + 10
    detail.save(os.path.join(a.out, f"{name}-variants-detail.png"))

    # Readability sheet: real size and 1:2 (left), the same enlarged 4x (middle), tinted (right).
    Z = 4
    col_real = (fw + 4) * len(show) + (fw // 2 + 4) * len(show) + 20
    col_zoom = Z * col_real
    tint_w = (fw + 4) * len(show)
    cell_h = fh * Z + 14
    sheet = Image.new("RGBA", (label_w + col_real + 20 + col_zoom + 20 + tint_w, 40 + cell_h * len(rows)), BG)
    d = ImageDraw.Draw(sheet)
    x_real, x_zoom, x_tint = label_w, label_w + col_real + 20, label_w + col_real + 20 + col_zoom + 20
    d.text((x_real, 6), "real size: 1x | 1:2", fill=LABEL)
    d.text((x_zoom, 6), "the same, enlarged 4x (nearest neighbour)", fill=LABEL)
    d.text((x_tint, 6), "team-tinted (green), 1x", fill=LABEL)
    for r, (label, sheets, overlay) in enumerate(rows):
        y = 40 + r * cell_h
        d.text((6, y + 8), label, fill=TEXT)
        strip = Image.new("RGBA", (col_real, fh + 2), BG)
        x = 0
        for i in show:
            strip.alpha_composite(crop(sheets[1], frames[i], 1), (x, 0))
            x += fw + 4
        x += 16
        for i in show:
            strip.alpha_composite(half(crop(sheets[1], frames[i], 1)), (x, 0))
            x += fw // 2 + 4
        sheet.alpha_composite(strip, (x_real, y))
        sheet.alpha_composite(strip.resize((strip.width * Z, strip.height * Z), Image.NEAREST),
                              (x_zoom, y))
        tint_sheet = tinted(sheets[1], overlay)
        x = x_tint
        for i in show:
            sheet.alpha_composite(crop(tint_sheet, frames[i], 1), (x, y))
            x += fw + 4
    sheet.save(os.path.join(a.out, f"{name}-variants-readability.png"))
    print(f"wrote {name}-variants-detail.png ({detail.width}x{detail.height}) and"
          f" {name}-variants-readability.png ({sheet.width}x{sheet.height}) in {a.out}")


if __name__ == "__main__":
    main()
