#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Make a side-by-side comparison sheet of an original sprite and its re-rendered version.

    python tools/sprite-render/compare_sheet.py --sprite uns/cruiser [--out out/sprite-render]

Needs Pillow. For every frame it shows, top to bottom: the original 1x image, the new 1x image,
the new overlay (team-colour brightness in red), all enlarged 4x with no smoothing, and the new
4x image at its real size. A red dot marks the frame's pivot. Writes compare.png next to the
rendered sheets.
"""

import argparse
import os
import re

from PIL import Image, ImageDraw

FRAME_RE = re.compile(
    r"left:\s*(-?\d+),\s*top:\s*(-?\d+),\s*right:\s*(-?\d+),\s*bottom:\s*(-?\d+),"
    r"\s*cx:\s*(-?\d+),\s*cy:\s*(-?\d+)")
ZOOM = 4
BG = (24, 24, 28, 255)


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("--sprite", required=True)
    p.add_argument("--data", default="data")
    p.add_argument("--out", default="out/sprite-render")
    p.add_argument("--per-row", type=int, default=8, help="frames per row (default: 8)")
    a = p.parse_args()

    with open(os.path.join(a.data, "sprites", a.sprite + ".pn"), encoding="utf-8") as f:
        frames = [tuple(map(int, m)) for m in FRAME_RE.findall(f.read())]
    new_dir = os.path.join(a.out, "sprites", a.sprite)
    sources = [
        ("original", Image.open(os.path.join(a.data, "sprites", a.sprite, "image.png")).convert("RGBA"), 1),
        ("new 1x", Image.open(os.path.join(new_dir, "image.png")).convert("RGBA"), 1),
        ("new overlay", Image.open(os.path.join(new_dir, "overlay.png")).convert("RGBA"), 1),
        ("new 4x", Image.open(os.path.join(new_dir, "image@4x.png")).convert("RGBA"), 4),
    ]

    cell_w = max(r - l for l, t, r, b, cx, cy in frames) * ZOOM + 12
    cell_h = max(b - t for l, t, r, b, cx, cy in frames) * ZOOM + 8
    label_w, header_h = 92, 18
    rows = (len(frames) + a.per_row - 1) // a.per_row
    block_h = header_h + cell_h * len(sources) + 10
    sheet = Image.new("RGBA", (label_w + cell_w * a.per_row, block_h * rows), BG)
    draw = ImageDraw.Draw(sheet)

    for i, (l, t, r, b, cx, cy) in enumerate(frames):
        col, row = i % a.per_row, i // a.per_row
        x0, y0 = label_w + col * cell_w, row * block_h
        draw.text((x0 + 2, y0 + 3), f"{i}  {i * 360 // len(frames)} deg", fill=(230, 210, 90, 255))
        for k, (name, img, s) in enumerate(sources):
            crop = img.crop((l * s, t * s, r * s, b * s))
            if s < ZOOM:
                crop = crop.resize(((r - l) * ZOOM, (b - t) * ZOOM), Image.NEAREST)
            y = y0 + header_h + k * cell_h
            sheet.alpha_composite(crop, (x0 + 4, y))
            px, py = x0 + 4 + (cx - l) * ZOOM, y + (cy - t) * ZOOM
            draw.ellipse((px - 2, py - 2, px + 2, py + 2), fill=(255, 40, 40, 255))
            if col == 0:
                draw.text((6, y + 4), name, fill=(200, 200, 200, 255))

    path = os.path.join(new_dir, "compare.png")
    sheet.save(path)
    print(f"wrote {path} ({sheet.width}x{sheet.height})")


if __name__ == "__main__":
    main()
