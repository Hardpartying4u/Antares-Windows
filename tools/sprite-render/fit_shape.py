#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Fit a parametric ship shape to an original sprite's frames.

    python tools/sprite-render/fit_shape.py --sprite uns/cruiser --shape uns_cruiser_shape \
        --tilt 44 --distance 200 --out tools/sprite-render/uns_cruiser_fit.json

Voxelises the shape (shape.occupancy), projects it into all frames with the fitted camera at 3x3
sub-pixel resolution (coverage >= 50% counts, like the renderer's alpha), and maximises the mean
silhouette IoU against the originals with Powell's method, penalising anything that spills out of
a frame. Writes the fitted parameters and the fit numbers as JSON. Needs numpy, scipy, Pillow.
"""

import argparse
import importlib
import json
import os
import time

import numpy as np
from scipy.optimize import minimize

import fit_metric
from sprite_geometry import Camera, Sprite, project

SUB = 3  # sub-pixels per pixel along each axis (--sub); keep the grid step at most 1 / SUB, or
# axis-aligned frames get empty sub-pixel stripes


class Fitter:
    def __init__(self, sprite, cam, shape, step, bounds):
        self.sprite, self.cam, self.shape = sprite, cam, shape
        (x0, x1), (y0, y1), (z0, z1) = bounds
        self.xs, self.ys, self.zs = (np.arange(x0, x1, step), np.arange(y0, y1, step),
                                     np.arange(z0, z1, step))
        gx, gy, gz = np.meshgrid(self.xs, self.ys, self.zs, indexing="ij")
        self.pts = np.stack([gx.ravel(), gy.ravel(), gz.ravel()], axis=1)
        self.targets = [sprite.frame_mask(i) for i in range(sprite.n)]
        # Precompute, for every grid point and frame, the sub-pixel it lands in (-1 = outside).
        self.cells = []
        for i in range(sprite.n):
            h, w = self.targets[i].shape
            fx, fy, _ = project(self.pts, sprite, i, cam)
            ix, iy = np.floor(fx * SUB).astype(np.int32), np.floor(fy * SUB).astype(np.int32)
            ok = (ix >= 0) & (ix < w * SUB) & (iy >= 0) & (iy < h * SUB)
            self.cells.append(np.where(ok, iy * (w * SUB) + ix, -1).astype(np.int32))
        self.evals = 0
        self.smooth = 0.0

    def occupied(self, p):
        x = self.xs[:, None, None]
        y = self.ys[None, :, None]
        z = self.zs[None, None, :]
        return self.shape.occupancy(p, x, y, z).ravel()

    def masks(self, p):
        """Rendered-like coverage per pixel and the share of the ship outside each frame."""
        occ = self.occupied(p)
        n = max(1, occ.sum())
        out = []
        for i in range(self.sprite.n):
            h, w = self.targets[i].shape
            c = self.cells[i][occ]
            inside = c[c >= 0]
            sub = np.zeros(h * SUB * w * SUB, bool)
            sub[inside] = True
            cover = sub.reshape(h, SUB, w, SUB).mean(axis=(1, 3))
            out.append((cover, (len(c) - len(inside)) / n))
        return out

    def score(self, p):
        """Soft IoU (coverage against the binary original), minus a spill penalty."""
        self.evals += 1
        total, spill_pen = 0.0, 0.0
        for (cover, spill), a in zip(self.masks(p), self.targets):
            af = a.astype(float)
            total += np.minimum(af, cover).sum() / max(1e-9, np.maximum(af, cover).sum())
            spill_pen += spill
        reg = self.shape.roughness_penalty(p) if hasattr(self.shape, "roughness_penalty") else 0.0
        return total / self.sprite.n - 2.0 * spill_pen / self.sprite.n - self.smooth * reg

    def evaluate(self, p):
        rows = []
        for (cover, spill), a in zip(self.masks(p), self.targets):
            r = fit_metric.compare(a, cover >= 0.5)
            r["spill"] = float(spill)
            rows.append(r)
        return rows


def to_vec(p, names):
    return np.array([p[n] for n in names])


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--sprite", required=True)
    ap.add_argument("--data", default="data")
    ap.add_argument("--shape", required=True, help="shape module, e.g. uns_cruiser_shape")
    ap.add_argument("--tilt", type=float, required=True)
    ap.add_argument("--distance", type=float)
    ap.add_argument("--step", type=float, default=0.25)
    ap.add_argument("--bounds", default="-11,11,-28,17,-13,5")
    ap.add_argument("--rounds", type=int, default=3)
    ap.add_argument("--sub", type=int, default=3, help="sub-pixels per pixel along each axis")
    ap.add_argument("--smooth", type=float, default=0.002, help="weight of the shape's roughness penalty")
    ap.add_argument("--init", help="JSON from an earlier fit to start from")
    ap.add_argument("--out", required=True)
    a = ap.parse_args()

    global SUB
    SUB = a.sub
    if a.step > 1.0 / SUB + 1e-9:
        print(f"warning: grid step {a.step} is coarser than a sub-pixel (1/{SUB}); expect stripes")
    shape = importlib.import_module(a.shape)
    sprite = Sprite(a.data, a.sprite)
    cam = Camera(a.tilt, a.distance)
    b = [float(v) for v in a.bounds.split(",")]
    fitter = Fitter(sprite, cam, shape, a.step, [(b[0], b[1]), (b[2], b[3]), (b[4], b[5])])
    names = shape.NAMES
    lo = np.array([shape.PARAMS[n][1] for n in names])
    hi = np.array([shape.PARAMS[n][2] for n in names])
    fitter.smooth = a.smooth
    p0 = shape.initial()
    if a.init:
        with open(a.init, encoding="utf-8") as f:
            p0.update({k: v for k, v in json.load(f)["params"].items() if k in p0})
        for n in names:  # keep the start inside the (possibly widened) bounds
            p0[n] = float(np.clip(p0[n], shape.PARAMS[n][1], shape.PARAMS[n][2]))
    start = time.time()
    s0 = fitter.score(p0)

    def objective(v):
        return -fitter.score({**p0, **dict(zip(names, np.clip(v, lo, hi)))})

    v = to_vec(p0, names)
    for r in range(a.rounds):
        res = minimize(objective, v, method="Powell", bounds=list(zip(lo, hi)),
                       options={"xtol": 0.02, "ftol": 1e-5, "maxfev": 4000})
        v = np.clip(res.x, lo, hi)
        print(f"round {r + 1}: score {-res.fun:.4f} after {fitter.evals} evaluations,"
              f" {time.time() - start:.0f} s", flush=True)
    p = {**{k: float(x) for k, x in p0.items()}, **dict(zip(names, [float(x) for x in v]))}
    rows = fitter.evaluate(p)
    summary = {
        "iou_mean": float(np.mean([r["iou"] for r in rows])),
        "iou_worst": float(min(r["iou"] for r in rows)),
        "edge_mean": float(np.mean([r["edge_mean"] for r in rows])),
        "edge_p95": float(max(r["edge_p95"] for r in rows)),
        "spill_max": float(max(r["spill"] for r in rows)),
    }
    out = {"sprite": a.sprite, "shape": a.shape, "camera": {"tilt": a.tilt, "distance": a.distance},
           "score_initial": s0, "score": fitter.score(p), "summary": summary, "params": p,
           "seconds": round(time.time() - start), "evaluations": fitter.evals}
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    with open(a.out, "w", encoding="utf-8") as f:
        json.dump(out, f, indent=2)
    print(json.dumps({k: out[k] for k in ("score_initial", "score", "summary", "seconds")}, indent=2))


if __name__ == "__main__":
    main()
