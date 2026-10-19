#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""1x image sheets from the AI @2x sheets, so Graphics: Enhanced shows them at UI 1x too.

    python tools/sprite-render/shrink_ai_1x.py --decisions tools/sprite-render/upscale_decisions.json \
        --art art --out out/sprite-render/upscale/ai1x

For every sprite whose image method is "ai": average each 2x2 block of art/<sprite>/image@2x.png
with premultiplied alpha (an exact 2:1 area filter), then give the result the original 1x alpha,
so the outline stays exactly the original's (it must match the original 1x overlay, which is kept,
and collision bounds) and transparent pixels can't darken the edges. An opaque 1x pixel whose
2x2 block has no opaque pixel keeps its original colour (counted as "fallback").

Writes <out>/sprites/<sprite>/image.png and <out>/report.json with, inside the ship, against the
original 1x image:
  agreement_orig/new  correlation of image brightness with the original overlay shade (red); the
                      overlay is what a team tint draws, so the image should still follow it.
  edge_luma           mean brightness of the outline pixels, new / original (below 1: dark fringe).
  edge_recall         on the original's strong, coherently oriented lines, the share of their 3x3
                      contrast the new image keeps (low: mushy edges).
  light_recall        share of the original's small bright or saturated spots (lights) still
                      standing out within one pixel (low: lost lights).
  fallback            opaque pixels that kept their original colour.
Flags: agreement (drop > --max-drop), fringe (edge_luma < --min-edge-luma), mushy (edge_recall <
--min-edge), lights (light_recall < --min-lights). Flags only guide the visual review. Needs numpy,
scipy, Pillow.
"""

import argparse
import json
import os

import numpy as np
from PIL import Image
from scipy import ndimage

from upscale_report import corr, load, lum, spots


def shrink(hi, alpha1x):
    """2:1 area reduction with premultiplied alpha; returns RGBA with the given 1x alpha."""
    h, w = alpha1x.shape
    a = hi[..., 3].astype(float) / 255
    pre = hi[..., :3].astype(float) * a[..., None]
    blk = lambda x: x[:2 * h, :2 * w].reshape(h, 2, w, 2, *x.shape[2:]).sum(axis=(1, 3))
    asum = blk(a)
    rgb = blk(pre) / np.maximum(asum, 1e-9)[..., None]
    out = np.zeros((h, w, 4), np.uint8)
    out[..., :3] = np.clip(np.rint(rgb), 0, 255)
    out[..., 3] = alpha1x
    return out, (alpha1x > 0) & (asum == 0)


def measure(orig, new, overlay):
    mask = orig[..., 3] > 0
    inner = ndimage.binary_erosion(mask)
    rim = mask & ~inner
    m = {}
    m["agreement_orig"] = corr(lum(orig)[mask], overlay[..., 0][mask].astype(float))
    m["agreement_new"] = corr(lum(new)[mask], overlay[..., 0][mask].astype(float))
    m["agreement_drop"] = m["agreement_orig"] - m["agreement_new"]
    m["edge_luma"] = float(lum(new)[rim].mean() / max(lum(orig)[rim].mean(), 1e-9)) if rim.any() else 1.0
    L = lum(orig)
    gx, gy = ndimage.sobel(L, 1), ndimage.sobel(L, 0)
    jxx, jyy, jxy = (ndimage.gaussian_filter(v, 1.0) for v in (gx * gx, gy * gy, gx * gy))
    coherence = np.sqrt((jxx - jyy) ** 2 + 4 * jxy ** 2) / (jxx + jyy + 1e-9)
    lines = inner & (np.hypot(gx, gy) > 60) & (coherence > 0.55)
    rng = lambda x: ndimage.maximum_filter(x, 3) - ndimage.minimum_filter(x, 3)
    a_, b_ = rng(L)[lines], rng(lum(new))[lines]
    m["edge_recall"] = float(np.minimum(a_, b_).sum() / max(a_.sum(), 1e-9)) if lines.sum() >= 10 else 1.0
    so = spots(orig, mask)
    sn = ndimage.maximum_filter(spots(new, mask), 3)
    m["light_recall"] = float((so & sn).sum() / max(so.sum(), 1)) if so.sum() >= 5 else 1.0
    m["lights"] = int(so.sum())
    return m


def main():
    a = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    a.add_argument("--data", default="data")
    a.add_argument("--art", default="art")
    a.add_argument("--decisions", required=True)
    a.add_argument("--out", required=True)
    a.add_argument("--max-drop", type=float, default=0.10)
    a.add_argument("--min-edge-luma", type=float, default=0.90)
    a.add_argument("--min-edge", type=float, default=0.80)
    a.add_argument("--min-lights", type=float, default=0.60)
    args = a.parse_args()
    decisions = json.load(open(args.decisions, encoding="utf-8"))
    decisions.pop("_legend", None)
    report = {}
    for s in sorted(k for k, v in decisions.items() if v["image"] == "ai"):
        orig = load(f"{args.data}/sprites/{s}/image.png")
        overlay = load(f"{args.data}/sprites/{s}/overlay.png")
        hi = load(f"{args.art}/sprites/{s}/image@2x.png")
        assert hi.shape[:2] == (2 * orig.shape[0], 2 * orig.shape[1]), s
        new, fb = shrink(hi, orig[..., 3])
        os.makedirs(f"{args.out}/sprites/{s}", exist_ok=True)
        Image.fromarray(new).save(f"{args.out}/sprites/{s}/image.png")
        m = measure(orig, new, overlay)
        m["fallback"] = int(fb.sum())
        f = []
        if np.isfinite(m["agreement_drop"]) and m["agreement_drop"] > args.max_drop:
            f.append("agreement")
        if m["edge_luma"] < args.min_edge_luma:
            f.append("fringe")
        if m["edge_recall"] < args.min_edge:
            f.append("mushy")
        if m["light_recall"] < args.min_lights:
            f.append("lights")
        m["flags"] = f
        report[s] = m
        print(f"{s:24} agree {m['agreement_orig']:.2f}->{m['agreement_new']:.2f}  edge_luma {m['edge_luma']:.2f}"
              f"  edges {m['edge_recall']:.2f}  lights {m['light_recall']:.2f} ({m['lights']})"
              f"  fallback {m['fallback']}  {' '.join(f)}")
    json.dump(report, open(f"{args.out}/report.json", "w"), indent=1)
    print(f"{len(report)} sprites, {sum(1 for m in report.values() if m['flags'])} flagged")


if __name__ == "__main__":
    main()
