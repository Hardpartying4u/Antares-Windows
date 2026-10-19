#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Contact sheets by faction: originals beside upscaled sheets at 2x, untinted and team-tinted.

    python tools/sprite-render/upscale_contact.py --report out/sprite-render/upscale/report.json \
        --ai out/sprite-render/upscale/ai-realesr-animevideov3 --tolerant out/sprite-render/upscale/tol24 \
        --out out/sprite-render/upscale/contact

One row per sprite, frame at 45 degrees, at the size the game draws it at UI 2x (capital ships
2x again): original (pixel-doubled, as the game draws it today) | AI upscale, untinted and in two
team hues (the game's own tint: tint_sheet.py). Flagged sprites are marked in red with their
reasons, and show the tolerant Scale2x upscale too; sprites with a 3D remaster in art/ show it.
The measured numbers (upscale_report.py) are printed on each row. Needs numpy and Pillow.
"""

import argparse
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
HUES = ("gray", "red", "blue")


def load(p):
    return np.asarray(Image.open(p).convert("RGBA"))


def main():
    a = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    a.add_argument("--data", default="data")
    a.add_argument("--report", required=True)
    a.add_argument("--ai", required=True)
    a.add_argument("--tolerant", required=True)
    a.add_argument("--art", default="art")
    a.add_argument("--manual", help="JSON {sprite: reason} of flags from visual review")
    a.add_argument("--inventory", default="out/sprite-render/fleet/inventory.csv")
    a.add_argument("--out", required=True)
    args = a.parse_args()
    report = json.load(open(args.report, encoding="utf-8"))
    manual = json.load(open(args.manual, encoding="utf-8")) if args.manual else {}
    import csv
    scale = {r["sprite"]: float(r["draw_scale"]) for r in csv.DictReader(open(args.inventory, encoding="utf-8"))}
    try:
        font, small = ImageFont.truetype("segoeui.ttf", 15), ImageFont.truetype("segoeui.ttf", 12)
    except OSError:
        font = small = ImageFont.load_default()
    os.makedirs(args.out, exist_ok=True)
    groups = defaultdict(list)
    for s in report:
        groups[s.split("/")[0]].append(s)
    for fac, sprites in sorted(groups.items()):
        rows = []
        for s in sorted(sprites):
            ai = report[s].get("ai")
            if ai is None:
                continue
            flags = list(ai["flags"]) + ([f"manual: {manual[s]}"] if s in manual else [])
            fr = fit_metric.read_frames(args.data, s)
            l, t, r, b, _, _ = fr[len(fr) // 8]
            k = 2 * (2 if scale.get(s, 1) >= 2 else 1) if scale.get(s, 1) < 4 else 2 * 4
            sheet_k = min(4, 2 if k <= 2 else 4)  # which upscaled sheet the game would pick
            o_i, o_o = load(f"{args.data}/sprites/{s}/image.png"), load(f"{args.data}/sprites/{s}/overlay.png")
            sources = [("original", None), ("AI", args.ai)]
            if flags:
                sources.append(("tolerant", args.tolerant))
            if os.path.exists(f"{args.art}/sprites/{s}/image@{sheet_k}x.png"):
                sources.append(("3D (art/)", args.art))
            tiles = []
            for hue in HUES:
                for name, folder in sources:
                    if folder is None:
                        im = Image.fromarray(tinted(o_i, o_o, hue)[t:b, l:r]).resize(((r - l) * k, (b - t) * k), Image.NEAREST)
                    else:
                        n_i, n_o = load(f"{folder}/sprites/{s}/image@{sheet_k}x.png"), load(f"{folder}/sprites/{s}/overlay@{sheet_k}x.png")
                        im = Image.fromarray(tinted(n_i, n_o, hue)[t * sheet_k:b * sheet_k, l * sheet_k:r * sheet_k])
                        if sheet_k != k:
                            im = im.resize(((r - l) * k, (b - t) * k), Image.LANCZOS)
                    cell = Image.new("RGBA", (max(im.width, 70), im.height + 16), (14, 14, 20, 255))
                    ImageDraw.Draw(cell).text((1, 0), f"{name} {hue}", font=small, fill=(170, 170, 185))
                    cell.alpha_composite(im, (0, 16))
                    tiles.append(cell)
                tiles.append(None)  # gap between hues
            W = sum(c.width + 6 if c else 18 for c in tiles) + 12
            H = max(c.height for c in tiles if c) + 44
            row = Image.new("RGBA", (W, H), (14, 14, 20, 255))
            d = ImageDraw.Draw(row)
            colour = (255, 90, 70) if flags else (225, 225, 232)
            tol = report[s].get("tolerant")
            nums = (f"AI: overlay fidelity {ai['overlay_fidelity']:.3f}, agreement {ai['agreement_orig']:.3f}->"
                    f"{ai['agreement_new']:.3f}, structure kept {ai['edge_recall']:.2f}")
            if tol:
                nums += f" | tolerant: fidelity {tol['overlay_fidelity']:.3f}, structure kept {tol['edge_recall']:.2f}"
            d.text((6, 2), f"{s}  (drawn at {k}x here)" + (f"  FLAGGED: {'; '.join(flags)}" if flags else ""),
                   font=font, fill=colour)
            d.text((6, 22), nums, font=small, fill=(150, 150, 165))
            x = 6
            for c in tiles:
                if c is None:
                    x += 18
                    continue
                row.alpha_composite(c, (x, 40))
                x += c.width + 6
            if flags:
                d.rectangle((0, 0, W - 1, H - 1), outline=(255, 90, 70), width=2)
            rows.append(row)
        if not rows:
            continue
        W = max(r.width for r in rows)
        H = sum(r.height + 6 for r in rows) + 40
        out = Image.new("RGB", (W, H), (10, 10, 16))
        d = ImageDraw.Draw(out)
        n_flag = sum(1 for s in sprites if report[s].get("ai", {}).get("flags") or s in manual)
        d.text((8, 8), f"{FACTIONS.get(fac, fac)}: original vs AI upscale (Real-ESRGAN realesr-animevideov3) at UI 2x,"
                       f" untinted / red / blue team hue. {len(rows)} sprites, {n_flag} flagged.", font=font,
               fill=(230, 230, 236))
        y = 36
        for r in rows:
            out.paste(r, (0, y))
            y += r.height + 6
        out.save(os.path.join(args.out, f"{fac}.png"))
        print(f"{fac}: {len(rows)} sprites, {n_flag} flagged -> {fac}.png ({W}x{H})")


if __name__ == "__main__":
    main()
