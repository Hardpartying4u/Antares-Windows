#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Appearance metric: how closely a remaster's colours, contrast and structure match the original.

    python tools/sprite-render/appearance_metric.py --sprite ish/gunship --shape ish_gunship_shape \
        --render out/sprite-render/remaster --carve out/sprite-render/carve-ish-gunship

The companion to fit_metric.py (which compares outlines). Two parts, both at 1x:

  Per colour region (hull, pods, block, ...): the original's and the render's colours are
  back-projected onto the same carved surface points (carve.py) and labelled by the ship's shape
  module. For each region: mean colour difference (sRGB levels), and the brightness spread as
  10th / 50th / 90th percentiles, so a flat mid-tone can't pass for the original's dark gaps and
  bright highlights. "Range" is the render's p90 - p10 over the original's.

  Per frame: structural similarity (SSIM, Gaussian window sigma 1.5, on luminance inside the
  union of both silhouettes), mean absolute luminance difference, and contrast (luminance
  standard deviation) of the render over the original's.

Writes appearance-metric.md next to the render and prints the summary. Needs numpy, scipy, Pillow.
"""

import argparse
import importlib
import json
import os

import numpy as np
from PIL import Image
from scipy import ndimage

import fit_metric
from carve import visible_colours
from sprite_geometry import Camera, Sprite

C1, C2 = (0.01 * 255) ** 2, (0.03 * 255) ** 2


def luminance(rgb):
    return rgb[..., 0] * 0.299 + rgb[..., 1] * 0.587 + rgb[..., 2] * 0.114


def ssim(a, b, mask, sigma=1.5):
    """Mean SSIM of two luminance images over `mask` (Wang et al. 2004, Gaussian window)."""
    f = lambda x: ndimage.gaussian_filter(x, sigma)
    mu_a, mu_b = f(a), f(b)
    va, vb, cov = f(a * a) - mu_a ** 2, f(b * b) - mu_b ** 2, f(a * b) - mu_a * mu_b
    s = ((2 * mu_a * mu_b + C1) * (2 * cov + C2)) / ((mu_a ** 2 + mu_b ** 2 + C1) * (va + vb + C2))
    return float(s[mask].mean())


def composite(rgba):
    """Straight-alpha sprite over black, as the game draws it on space."""
    rgba = rgba.astype(float)
    return rgba[..., :3] * (rgba[..., 3:4] / 255.0)


def frame_metrics(frames, orig, new):
    rows = []
    for l, t, r, b, _, _ in frames:
        o, n = orig[t:b, l:r], new[t:b, l:r]
        mask = (o[..., 3] >= 128) | (n[..., 3] >= 128)
        lo, ln = luminance(composite(o)), luminance(composite(n))
        so, sn = lo[o[..., 3] >= 128], ln[n[..., 3] >= 128]
        rows.append({"ssim": ssim(lo, ln, mask), "lum_diff": float(np.abs(lo - ln)[mask].mean()),
                     "contrast": float(sn.std() / max(so.std(), 1e-6))})
    return rows


def region_metrics(shape, p, cam, sprite, render_image, carve_dir):
    d = np.load(os.path.join(carve_dir, "carve.npz"))
    pts = d["surf_pts"]
    ok = np.ones(len(pts), bool)
    co, so = visible_colours(pts, ok, sprite, cam)
    new = Sprite(sprite_data_dir, sprite_name)
    new.image = render_image
    cn, sn = visible_colours(pts, ok, new, cam)
    use = (so >= 3) & (sn >= 3) & (co[:, 3] > 200) & (cn[:, 3] > 200)
    label = shape.part_of(p, pts)
    out = {}
    for region in sorted(set(label[use])):
        m = use & (label == region)
        if m.sum() < 15:
            continue
        lo, ln = luminance(co[m, :3]), luminance(cn[m, :3])
        po, pn = np.percentile(lo, [10, 50, 90]), np.percentile(ln, [10, 50, 90])
        out[region] = {"points": int(m.sum()), "colour_diff": float(np.abs(co[m, :3].mean(0) - cn[m, :3].mean(0)).mean()),
                       "orig_mean": co[m, :3].mean(0).round().tolist(), "new_mean": cn[m, :3].mean(0).round().tolist(),
                       "orig_p": po.round().tolist(), "new_p": pn.round().tolist(),
                       "range": float((pn[2] - pn[0]) / max(po[2] - po[0], 1e-6))}
    return out


def main():
    global sprite_data_dir, sprite_name
    a = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    a.add_argument("--sprite", required=True)
    a.add_argument("--data", default="data")
    a.add_argument("--shape", required=True)
    a.add_argument("--fit", help="default: <ship>_fit.json next to the shape module")
    a.add_argument("--render", required=True)
    a.add_argument("--carve", required=True)
    a.add_argument("--out", help="default: <render>/sprites/<sprite>/appearance-metric.md")
    args = a.parse_args()
    sprite_data_dir, sprite_name = args.data, args.sprite

    shape = importlib.import_module(args.shape)
    here = os.path.dirname(os.path.abspath(__file__))
    with open(args.fit or os.path.join(here, args.shape.replace("_shape", "_fit.json")), encoding="utf-8") as f:
        fit = json.load(f)
    cam = Camera(fit["camera"]["tilt"], fit["camera"]["distance"])
    sprite = Sprite(args.data, args.sprite)
    base = os.path.join(args.render, "sprites", args.sprite)
    render_image = np.asarray(Image.open(os.path.join(base, "image.png")).convert("RGBA"))
    frames = fit_metric.read_frames(args.data, args.sprite)

    regions = region_metrics(shape, fit["params"], cam, sprite, render_image, args.carve)
    rows = frame_metrics(frames, sprite.image, render_image)
    ss = [r["ssim"] for r in rows]
    lines = [f"# Appearance: {args.sprite}", "",
             f"- SSIM (1x, luminance): mean {np.mean(ss):.3f}, worst {min(ss):.3f}",
             f"- Mean luminance difference: {np.mean([r['lum_diff'] for r in rows]):.1f} levels",
             f"- Contrast (render / original luminance spread): {np.mean([r['contrast'] for r in rows]):.2f}", "",
             "| Region | Points | Colour diff | Original mean RGB | Render mean RGB | Original p10/p50/p90 | Render p10/p50/p90 | Range |",
             "|---|---:|---:|---|---|---|---|---:|"]
    for name, r in regions.items():
        lines.append(f"| {name} | {r['points']} | {r['colour_diff']:.1f} | {r['orig_mean']} | {r['new_mean']}"
                     f" | {r['orig_p']} | {r['new_p']} | {r['range']:.2f} |")
    lines += ["", "| Frame | SSIM | Luminance diff | Contrast |", "|---:|---:|---:|---:|"]
    lines += [f"| {i} | {r['ssim']:.3f} | {r['lum_diff']:.1f} | {r['contrast']:.2f} |" for i, r in enumerate(rows)]
    text = "\n".join(lines) + "\n"
    with open(args.out or os.path.join(base, "appearance-metric.md"), "w", encoding="utf-8") as f:
        f.write(text)
    print("\n".join(lines[:5 + 2 + len(regions)]))


if __name__ == "__main__":
    main()
