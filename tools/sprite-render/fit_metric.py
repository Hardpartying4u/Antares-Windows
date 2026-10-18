#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Silhouette fit metric: how closely a re-rendered sprite matches the original, frame by frame.

    python tools/sprite-render/fit_metric.py --sprite uns/cruiser --render out/sprite-render/remaster

Compares the 1x silhouettes (alpha >= 50%) of the original and the new image.png inside each
original frame rectangle. Per frame:

  IoU        overlap / union of the two silhouettes (1.0 = identical)
  extra      pixels covered only by the new sprite
  missing    pixels covered only by the original
  edge mean  mean distance between the two outlines, in pixels, both ways (0 = identical)
  edge p95   95th percentile of that distance (the worst stretches of outline)
  shift      offset of the new silhouette's centroid from the original's, in pixels

At these sizes IoU is harsh: a uniform 1 px error around a 20 x 34 px frame costs about 20%
IoU, so the edge distances are the more intuitive measure. Needs numpy, scipy and Pillow.
"""

import argparse
import os
import re

import numpy as np
from PIL import Image
from scipy import ndimage

FRAME_RE = re.compile(
    r"left:\s*(-?\d+),\s*top:\s*(-?\d+),\s*right:\s*(-?\d+),\s*bottom:\s*(-?\d+),"
    r"\s*cx:\s*(-?\d+),\s*cy:\s*(-?\d+)")

# Targets for a remaster. For uns/cruiser the carved visual hull (the most any rigid shape can
# reach against these pixel-rounded frames, but lumpy) reprojects at mean IoU 0.97, worst 0.94,
# edge 0.12 px; a clean smooth shape fitted to the frames reaches about 0.90 / 0.87 / 0.48 px.
# Earlier hand-built variants scored 0.79 / 0.73 / 0.92 px.
TARGETS = {"iou_mean": 0.88, "iou_worst": 0.85, "edge_mean": 0.5, "edge_p95": 1.5}


def read_frames(data, sprite):
    with open(os.path.join(data, "sprites", sprite + ".pn"), encoding="utf-8") as f:
        return [tuple(map(int, m)) for m in FRAME_RE.findall(f.read())]


def mask(path):
    return np.asarray(Image.open(path).convert("RGBA"))[..., 3] >= 128


def _outline(m):
    return m & ~ndimage.binary_erosion(m, border_value=0)


def compare(a, b):
    """Metrics for two same-sized boolean silhouettes (a = original, b = new)."""
    union = (a | b).sum()
    iou = (a & b).sum() / union if union else 1.0
    ea, eb = _outline(a), _outline(b)
    if ea.any() and eb.any():
        # distance from each outline pixel to the nearest pixel of the other outline
        da = ndimage.distance_transform_edt(~eb)[ea]
        db = ndimage.distance_transform_edt(~ea)[eb]
        d = np.concatenate([da, db])
        edge_mean, edge_p95 = float(d.mean()), float(np.percentile(d, 95))
    else:
        edge_mean = edge_p95 = float("inf")
    ys, xs = np.mgrid[0:a.shape[0], 0:a.shape[1]]
    ca = (xs[a].mean(), ys[a].mean()) if a.any() else (0, 0)
    cb = (xs[b].mean(), ys[b].mean()) if b.any() else (0, 0)
    return {"iou": float(iou), "extra": int((b & ~a).sum()), "missing": int((a & ~b).sum()),
            "edge_mean": edge_mean, "edge_p95": edge_p95,
            "shift": (float(cb[0] - ca[0]), float(cb[1] - ca[1]))}


def evaluate(frames, original_mask, new_mask):
    rows = []
    for l, t, r, b, cx, cy in frames:
        rows.append(compare(original_mask[t:b, l:r], new_mask[t:b, l:r]))
    summary = {
        "iou_mean": float(np.mean([r["iou"] for r in rows])),
        "iou_worst": float(min(r["iou"] for r in rows)),
        "edge_mean": float(np.mean([r["edge_mean"] for r in rows])),
        "edge_p95": float(np.max([r["edge_p95"] for r in rows])),
    }
    return rows, summary


def report(sprite, rows, summary, title="Silhouette fit"):
    def mark(key, value, higher_better):
        ok = value >= TARGETS[key] if higher_better else value <= TARGETS[key]
        return f"{value:.2f} ({'meets' if ok else 'misses'} target {TARGETS[key]:g})"

    lines = [
        f"# {title}: {sprite}", "",
        f"- IoU: mean {mark('iou_mean', summary['iou_mean'], True)}, worst"
        f" {mark('iou_worst', summary['iou_worst'], True)}",
        f"- Outline distance: mean {mark('edge_mean', summary['edge_mean'], False)} px, worst"
        f" 95th percentile {mark('edge_p95', summary['edge_p95'], False)} px", "",
        "| Frame | Angle | IoU | Extra px | Missing px | Edge mean | Edge p95 | Shift x, y |",
        "|---:|---:|---:|---:|---:|---:|---:|---|",
    ]
    for i, r in enumerate(rows):
        lines.append(f"| {i} | {i * 360 // len(rows)} | {r['iou']:.2f} | {r['extra']} | {r['missing']}"
                     f" | {r['edge_mean']:.2f} | {r['edge_p95']:.1f} | {r['shift'][0]:+.1f}, {r['shift'][1]:+.1f} |")
    return "\n".join(lines) + "\n"


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("--sprite", required=True)
    p.add_argument("--data", default="data")
    p.add_argument("--render", required=True, help="render output root (contains sprites/<sprite>/)")
    p.add_argument("--out", help="markdown report path (default: <render>/sprites/<sprite>/fit-metric.md)")
    a = p.parse_args()
    frames = read_frames(a.data, a.sprite)
    orig = mask(os.path.join(a.data, "sprites", a.sprite, "image.png"))
    new = mask(os.path.join(a.render, "sprites", a.sprite, "image.png"))
    rows, summary = evaluate(frames, orig, new)
    text = report(a.sprite, rows, summary)
    out = a.out or os.path.join(a.render, "sprites", a.sprite, "fit-metric.md")
    with open(out, "w", encoding="utf-8") as f:
        f.write(text)
    print("\n".join(text.splitlines()[:5]))


if __name__ == "__main__":
    main()
