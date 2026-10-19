#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Team-tint comparison: originals and upscaled (or remastered) sheets as the game tints them.

    python tools/sprite-render/tint_sheet.py --sheets out/sprite-render/upscale/ai-realesr-animevideov3 \
        --sprites ish/gunship,uns/cruiser --out out/sprite-render/upscale/tint.png

Applies the game's own tint (NatePixTable::Frame::load_overlay and RgbColor::tint,
src/drawing/pix-table.cpp, src/drawing/color.cpp): the overlay's red channel is the shade, its
alpha the coverage, and the hue's diffuse colour scales the shade. Shows one frame per sprite,
untinted and in several hues, the original (pixel-doubled, as the game draws it without @2x/@4x)
beside the given sheets at 2x and 4x. Also prints how closely each sheet's overlay follows its
image (the original's own value is the reference). Needs numpy and Pillow.
"""

import argparse
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFont

import fit_metric

DIFFUSE = {"gray": (256, 256, 256), "red": (256, 0, 0), "blue": (0, 0, 256), "green": (0, 256, 0),
           "salmon": (256, 128, 128), "orange": (256, 128, 0), "aqua": (0, 256, 256)}


def tinted(image, overlay, hue):
    img = image.astype(int)
    if hue == "gray":
        return image
    d = np.array(DIFFUSE[hue])
    over = (overlay[..., 0:1].astype(int) * d[None, None, :]) // 256
    frac = overlay[..., 3:4].astype(int)
    out = img.copy()
    out[..., :3] = (over * frac + img[..., :3] * (255 - frac)) // 255
    return out.astype(np.uint8)


def load(path):
    return np.asarray(Image.open(path).convert("RGBA"))


def main():
    a = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    a.add_argument("--data", default="data")
    a.add_argument("--sheets", required=True, help="folder containing sprites/<sprite>/image@2x.png etc.")
    a.add_argument("--sprites", required=True)
    a.add_argument("--hues", default="gray,red,blue,green,salmon")
    a.add_argument("--frame", type=int, default=3)
    a.add_argument("--out", required=True)
    args = a.parse_args()
    hues = args.hues.split(",")
    try:
        font = ImageFont.truetype("segoeui.ttf", 13)
    except OSError:
        font = ImageFont.load_default()
    rows = []
    for s in args.sprites.split(","):
        fr = fit_metric.read_frames(args.data, s)
        l, t, r, b, _, _ = fr[args.frame]
        o_i, o_o = load(f"{args.data}/sprites/{s}/image.png"), load(f"{args.data}/sprites/{s}/overlay.png")
        m = o_i[..., 3] > 0
        ref = np.corrcoef(o_i[..., :3][m].mean(-1), o_o[..., 0][m])[0, 1]
        line = [f"{s}: overlay/image correlation original {ref:.3f}"]
        tiles = []
        for k in (2, 4):
            n_i, n_o = load(f"{args.sheets}/sprites/{s}/image@{k}x.png"), load(f"{args.sheets}/sprites/{s}/overlay@{k}x.png")
            mm = n_i[..., 3] > 0
            line.append(f"@{k}x {np.corrcoef(n_i[..., :3][mm].mean(-1), n_o[..., 0][mm])[0, 1]:.3f}")
            for hue in hues:
                orig = tinted(o_i, o_o, hue)[t:b, l:r]
                orig = Image.fromarray(orig).resize(((r - l) * k, (b - t) * k), Image.NEAREST)
                new = Image.fromarray(tinted(n_i, n_o, hue)[t * k:b * k, l * k:r * k])
                pair = Image.new("RGBA", (orig.width * 2 + 4, orig.height + 18), (14, 14, 20, 255))
                ImageDraw.Draw(pair).text((2, 0), f"{hue} {k}x", font=font, fill=(190, 190, 200))
                pair.alpha_composite(orig, (0, 18))
                pair.alpha_composite(new, (orig.width + 4, 18))
                tiles.append(pair)
        print(", ".join(line))
        W = sum(x.width for x in tiles) + 8 * len(tiles)
        H = max(x.height for x in tiles) + 20
        row = Image.new("RGBA", (W, H), (14, 14, 20, 255))
        ImageDraw.Draw(row).text((4, 2), ", ".join(line) + "   (each pair: original | new)", font=font,
                                  fill=(225, 225, 232))
        x = 0
        for tile in tiles:
            row.alpha_composite(tile, (x, 20))
            x += tile.width + 8
        rows.append(row)
    W = max(r.width for r in rows)
    H = sum(r.height + 6 for r in rows)
    sheet = Image.new("RGB", (W, H), (14, 14, 20))
    y = 0
    for r in rows:
        sheet.paste(r, (0, y))
        y += r.height + 6
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    sheet.save(args.out)
    print(f"wrote {args.out} ({sheet.width}x{sheet.height})")


if __name__ == "__main__":
    main()
