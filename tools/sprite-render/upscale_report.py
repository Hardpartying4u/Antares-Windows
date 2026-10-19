#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Measure upscaled sprite sheets against their originals and flag problems.

    python tools/sprite-render/upscale_report.py --methods ai=out/sprite-render/upscale/ai-realesr-animevideov3,\
tolerant=out/sprite-render/upscale/tol24 --sprites-from out/sprite-render/fleet/inventory.csv \
        --out out/sprite-render/upscale/report.json

For each sprite and method, on the @2x sheet reduced back to 1x (2x2 average) where a comparison
with the original needs it:
  overlay_fidelity  correlation of the reduced overlay shade (red) with the original overlay's,
                    inside the ship. Under a team hue the overlay is all that shows.
  agreement_drop    how much the overlay/image correlation fell from the original's (the original
                    value is the reference; ships whose overlay doesn't follow the image anyway,
                    like the Salrilian cruiser, are judged by overlay_fidelity instead).
  edge_recall       where the original has strong, coherently oriented edges (barrels, seams; dither
                    has no consistent orientation), the share of its local contrast the @2x sheet
                    keeps. Coarse: catches blurred barrels, misses one-pixel ribs.
  spot_recall       share of the original's small saturated or bright spots (lights, accents)
                    that the reduced upscale still has (information only: dither speckle counts too).
  sizes_ok          @2x and @4x exactly 2 and 4 times the 1x sheet; alpha strictly 0 or 255.
Flags: overlay (fidelity < --min-fidelity or drop > --max-drop), softened (edge_recall <
--min-edge). Detail loss in one-pixel patterns (ribs) needs a visual review. Writes JSON. Needs numpy, scipy, Pillow.
"""

import argparse
import csv
import json
import os

import numpy as np
from PIL import Image
from scipy import ndimage

import fit_metric


def load(p):
    return np.asarray(Image.open(p).convert("RGBA"))


def lum(a):
    return a[..., :3].astype(float) @ np.array([0.299, 0.587, 0.114])


def reduce2(a):
    h, w = a.shape[0] // 2 * 2, a.shape[1] // 2 * 2
    return a[:h, :w].astype(float).reshape(h // 2, 2, w // 2, 2, -1).mean(axis=(1, 3))


def corr(a, b):
    if a.std() < 1e-6 or b.std() < 1e-6:
        return float("nan")
    return float(np.corrcoef(a, b)[0, 1])


def spots(img, mask):
    """Small saturated or bright features: pixels standing out from their 3x3 neighbourhood."""
    rgb = img[..., :3].astype(float)
    sat = rgb.max(-1) - rgb.min(-1)
    L = lum(img)
    med = ndimage.median_filter(L, 3)
    smed = ndimage.median_filter(sat, 3)
    return mask & (((sat - smed) > 70) | ((L - med) > 70))


def measure(data, sprite, folder, args):
    o_i, o_o = load(f"{data}/sprites/{sprite}/image.png"), load(f"{data}/sprites/{sprite}/overlay.png")
    base = f"{folder}/sprites/{sprite}"
    n2_i, n2_o = load(f"{base}/image@2x.png"), load(f"{base}/overlay@2x.png")
    n4_i = load(f"{base}/image@4x.png")
    h, w = o_i.shape[:2]
    sizes_ok = (n2_i.shape[:2] == (2 * h, 2 * w) and n4_i.shape[:2] == (4 * h, 4 * w)
                and set(np.unique(n2_i[..., 3])) <= {0, 255} and set(np.unique(n4_i[..., 3])) <= {0, 255})
    mask = o_i[..., 3] > 0
    inner = ndimage.binary_erosion(mask, iterations=1)
    r_i, r_o = reduce2(n2_i), reduce2(n2_o)
    fid = corr(o_o[..., 0][mask].astype(float), r_o[..., 0][mask])
    a0 = corr(lum(o_i)[mask], o_o[..., 0][mask].astype(float))
    m2 = n2_i[..., 3] > 0
    a1 = corr(lum(n2_i)[m2], n2_o[..., 0][m2].astype(float))
    # strong inside edges
    # Softening of structural lines: where the original has strong, coherently oriented edges
    # (barrels, seams, panel lines; dither has no consistent orientation), how much of the 3x3
    # local contrast of the original (pixel-doubled) the @2x sheet keeps. Coarse: it catches
    # blurred barrels but not one-pixel ribs, which look like dither to any automatic measure.
    L = lum(o_i)
    gx, gy = ndimage.sobel(L, 1), ndimage.sobel(L, 0)
    jxx, jyy, jxy = (ndimage.gaussian_filter(v, 1.0) for v in (gx * gx, gy * gy, gx * gy))
    coherence = np.sqrt((jxx - jyy) ** 2 + 4 * jxy ** 2) / (jxx + jyy + 1e-9)
    lines = inner & (np.hypot(gx, gy) > args.edge) & (coherence > 0.55)
    lines2 = np.repeat(np.repeat(lines, 2, 0), 2, 1)
    rng = lambda x: ndimage.maximum_filter(x, 3) - ndimage.minimum_filter(x, 3)
    a_, b_ = rng(np.repeat(np.repeat(L, 2, 0), 2, 1))[lines2], rng(lum(n2_i))[lines2]
    edge_recall = float(np.minimum(a_, b_).sum() / max(a_.sum(), 1e-9)) if lines.sum() >= 10 else 1.0
    strong = lines
    so = spots(o_i, inner)
    sn = ndimage.maximum_filter(spots(np.clip(r_i, 0, 255).astype(np.uint8), inner), 3)
    spot_recall = float((so & sn).sum() / max(so.sum(), 1)) if so.sum() >= 5 else 1.0
    return {"overlay_fidelity": fid, "agreement_orig": a0, "agreement_new": a1,
            "agreement_drop": (a0 - a1) if np.isfinite(a0) and np.isfinite(a1) else float("nan"),
            "edge_recall": edge_recall, "strong_edges": int(strong.sum()), "spot_recall": spot_recall,
            "spots": int(so.sum()), "sizes_ok": bool(sizes_ok)}


def flags(m, args):
    f = []
    if m["overlay_fidelity"] < args.min_fidelity or (
            np.isfinite(m["agreement_drop"]) and m["agreement_orig"] > 0.5 and m["agreement_drop"] > args.max_drop):
        f.append("overlay")
    if m["edge_recall"] < args.min_edge:
        f.append("softened")
    if not m["sizes_ok"]:
        f.append("size/alpha")
    return f


def main():
    a = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    a.add_argument("--data", default="data")
    a.add_argument("--methods", required=True, help="name=folder,name=folder")
    a.add_argument("--sprites", help="comma-separated sprites")
    a.add_argument("--sprites-from", help="inventory.csv from fleet_inventory.py")
    a.add_argument("--edge", type=float, default=40.0)
    a.add_argument("--min-fidelity", type=float, default=0.90)
    a.add_argument("--max-drop", type=float, default=0.06)
    a.add_argument("--min-edge", type=float, default=0.80)
    a.add_argument("--min-spot", type=float, default=0.50)
    a.add_argument("--out", required=True)
    args = a.parse_args()
    methods = dict(kv.split("=", 1) for kv in args.methods.split(","))
    sprites = (args.sprites.split(",") if args.sprites else
               [r["sprite"] for r in csv.DictReader(open(args.sprites_from, encoding="utf-8"))])
    report = {}
    for s in sprites:
        report[s] = {}
        for name, folder in methods.items():
            if not os.path.exists(f"{folder}/sprites/{s}/image@2x.png"):
                continue
            m = measure(args.data, s, folder, args)
            m["flags"] = flags(m, args)
            report[s][name] = m
    with open(args.out, "w", encoding="utf-8") as f:
        json.dump(report, f, indent=1)
    for s, ms in report.items():
        print(s, " | ".join(f"{n}: fid {m['overlay_fidelity']:.3f} drop {m['agreement_drop']:+.3f} edges {m['edge_recall']:.2f}"
                            f" ({m['strong_edges']}) spots {m['spot_recall']:.2f} ({m['spots']}) {','.join(m['flags']) or 'ok'}"
                            for n, m in ms.items()))


if __name__ == "__main__":
    main()
