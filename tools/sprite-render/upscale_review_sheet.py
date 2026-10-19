#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Review sheets by faction: each sprite's original beside its chosen @2x result.

    python tools/sprite-render/upscale_review_sheet.py --decisions tools/sprite-render/upscale_decisions.json \
        --chosen out/sprite-render/upscale/hybrid --out out/sprite-render/upscale/review

One row per sprite at the size the game draws it at UI 2x (capital ships 2x again): original
(pixel-doubled, as today) | chosen result, untinted and in two team hues, using the game's tint
(tint_sheet.py). The row names the chosen image and overlay methods and the reason; unsure rows
are marked in amber. Sprites that stay original show the original on both sides. Needs numpy,
Pillow.
"""

import argparse
import csv
import json
import os
from collections import defaultdict

import numpy as np
from PIL import Image, ImageDraw, ImageFont

import fit_metric
from tint_sheet import tinted

FACTIONS = {"uns": "Human", "ish": "Ishiman", "can": "Cantharan", "gai": "Gaitori", "sal": "Salrilian",
            "aud": "Audemedon", "obi": "Obish", "baz": "Bazidan", "ele": "Elejeetian", "gro": "Grolk",
            "loc": "Location", "dev": "Weapons", "sfx": "Effects"}


def load(p):
    return np.asarray(Image.open(p).convert("RGBA"))


def main():
    a = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    a.add_argument("--data", default="data")
    a.add_argument("--decisions", required=True)
    a.add_argument("--chosen", required=True)
    a.add_argument("--inventory", default="out/sprite-render/fleet/inventory.csv")
    a.add_argument("--hues", default="gray,red,blue")
    a.add_argument("--out", required=True)
    args = a.parse_args()
    decisions = json.load(open(args.decisions, encoding="utf-8"))
    decisions.pop("_legend", None)
    scale = {r["sprite"]: float(r["draw_scale"]) for r in csv.DictReader(open(args.inventory, encoding="utf-8"))}
    try:
        font, small = ImageFont.truetype("segoeui.ttf", 15), ImageFont.truetype("segoeui.ttf", 12)
    except OSError:
        font = small = ImageFont.load_default()
    os.makedirs(args.out, exist_ok=True)
    groups = defaultdict(list)
    for s in decisions:
        groups[s.split("/")[0]].append(s)
    for fac, sprites in sorted(groups.items()):
        rows = []
        for s in sorted(sprites):
            c = decisions[s]
            fr = fit_metric.read_frames(args.data, s)
            l, t, r, b, _, _ = fr[len(fr) // 8]
            sc = scale.get(s, 1)
            k = 2 * int(max(1, sc))  # screen pixels per 1x pixel at UI 2x
            sheet_k = 2 if k <= 2 else 4
            o_i, o_o = load(f"{args.data}/sprites/{s}/image.png"), load(f"{args.data}/sprites/{s}/overlay.png")
            keep = c["image"] == "original"
            if not keep:
                n_i = load(f"{args.chosen}/sprites/{s}/image@{sheet_k}x.png")
                n_o = load(f"{args.chosen}/sprites/{s}/overlay@{sheet_k}x.png")
            tiles = []
            for hue in args.hues.split(","):
                for side in ("original", "chosen"):
                    if side == "original" or keep:
                        im = Image.fromarray(tinted(o_i, o_o, hue)[t:b, l:r]).resize(((r - l) * k, (b - t) * k), Image.NEAREST)
                    else:
                        im = Image.fromarray(tinted(n_i, n_o, hue)[t * sheet_k:b * sheet_k, l * sheet_k:r * sheet_k])
                        if sheet_k != k:
                            im = im.resize(((r - l) * k, (b - t) * k), Image.LANCZOS)
                    cell = Image.new("RGBA", (max(im.width, 64), im.height + 16), (14, 14, 20, 255))
                    ImageDraw.Draw(cell).text((1, 0), f"{side} {hue}", font=small, fill=(170, 170, 185))
                    cell.alpha_composite(im, (0, 16))
                    tiles.append(cell)
                tiles.append(None)
            W = sum(x.width + 6 if x else 16 for x in tiles) + 12
            H = max(x.height for x in tiles if x) + 44
            row = Image.new("RGBA", (W, H), (14, 14, 20, 255))
            d = ImageDraw.Draw(row)
            unsure = c.get("unsure", False)
            colour = (255, 190, 60) if unsure else (225, 225, 232)
            d.text((6, 2), f"{'UNSURE  ' if unsure else ''}{s}: image {c['image']}, overlay {c['overlay']}"
                           f"  (drawn at {k}x here)", font=font, fill=colour)
            d.text((6, 22), c["note"], font=small, fill=(150, 150, 165))
            x = 6
            for tile in tiles:
                if tile is None:
                    x += 16
                    continue
                row.alpha_composite(tile, (x, 40))
                x += tile.width + 6
            if unsure:
                d.rectangle((0, 0, W - 1, H - 1), outline=(255, 190, 60), width=2)
            rows.append(row)
        W = max(r.width for r in rows)
        H = sum(r.height + 6 for r in rows) + 40
        out = Image.new("RGB", (W, H), (10, 10, 16))
        n_unsure = sum(1 for s in sprites if decisions[s].get("unsure"))
        ImageDraw.Draw(out).text((8, 8), f"{FACTIONS.get(fac, fac)}: original vs chosen result at UI 2x (untinted, red,"
                                         f" blue team hue). {len(rows)} sprites, {n_unsure} unsure (amber).",
                                 font=font, fill=(230, 230, 236))
        y = 36
        for r in rows:
            out.paste(r, (0, y))
            y += r.height + 6
        out.save(os.path.join(args.out, f"{fac}.png"))
        print(f"{fac}: {len(rows)} sprites, {n_unsure} unsure -> {fac}.png ({W}x{H})")


if __name__ == "__main__":
    main()
