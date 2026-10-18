#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Find the camera an original sprite was rendered with: sweep tilt and distance, carve, compare.

    python tools/sprite-render/camera_sweep.py --sprite uns/fighter

For each camera (tilt from vertical, perspective distance or orthographic), carves the visual hull
from all frames (carve.py) and measures how well it reprojects onto them. The camera the original
was rendered with gives the most consistent hull, so the highest reprojection IoU wins. Bounds and
grid step follow the sprite's size. Prints a table and the best camera. Needs numpy, scipy, Pillow.
"""

import argparse
import math

import numpy as np

from carve import grid, reproject, votes
from sprite_geometry import Camera, Sprite


def auto_bounds(sprite, margin=1.15):
    """A cube around the pivot big enough for any frame (model units = 1x pixels)."""
    r = 0.0
    for l, t, rr, b, cx, cy in sprite.frames:
        for x, y in ((l, t), (rr, t), (l, b), (rr, b)):
            r = max(r, math.hypot(x - cx, y - cy))
    r = math.ceil(r * margin)
    return [(-r, r), (-r, r), (-r, r)], r


def score(sprite, cam, pts):
    count = votes(pts, sprite, cam)
    best = None
    for v in (sprite.n, sprite.n - 1, sprite.n - 2):
        occ = count >= v
        if occ.sum() < 10:
            continue
        _, s = reproject(pts[occ], sprite, cam)
        if best is None or s["iou_mean"] > best[1]["iou_mean"]:
            best = (v, s)
    return best


def main():
    a = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    a.add_argument("--sprite", required=True)
    a.add_argument("--data", default="data")
    a.add_argument("--tilts", default="20,28,36,40,44,48,52,60")
    a.add_argument("--distances", default="0,100,200,400", help="0 = orthographic")
    a.add_argument("--step", type=float, default=0, help="grid step (default: radius / 40)")
    args = a.parse_args()

    sprite = Sprite(args.data, args.sprite)
    bounds, r = auto_bounds(sprite)
    step = args.step or max(0.25, round(r / 40 * 4) / 4)
    _, pts = grid(bounds, step)
    print(f"{args.sprite}: radius {r}, step {step}, {len(pts)} grid points")
    print("| Tilt | Distance | Votes | IoU mean | IoU worst | Edge mean px |")
    print("|---:|---:|---:|---:|---:|---:|")
    results = []
    for d in (float(v) for v in args.distances.split(",")):
        for t in (float(v) for v in args.tilts.split(",")):
            best = score(sprite, Camera(t, d or None), pts)
            if best is None:
                continue
            v, s = best
            results.append((s["iou_mean"], t, d, s))
            print(f"| {t:g} | {d or 'ortho'} | {v} | {s['iou_mean']:.3f} | {s['iou_worst']:.3f}"
                  f" | {s['edge_mean']:.2f} |", flush=True)
    iou, t, d, s = max(results, key=lambda x: x[0])
    print(f"\nbest: tilt {t:g}, distance {d or 'ortho'}: IoU mean {iou:.3f}, worst {s['iou_worst']:.3f}")


if __name__ == "__main__":
    main()
