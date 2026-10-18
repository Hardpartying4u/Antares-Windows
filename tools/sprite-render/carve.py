#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Carve a 3D shape (visual hull) out of an original sprite's rotation frames.

    python tools/sprite-render/carve.py --sprite uns/cruiser --tilt 44 --distance 200 \
        --out out/sprite-render/carve

Every point of a voxel grid around the pivot is projected into all frames with the fitted camera
(see sprite_geometry.py) and kept if it falls inside the original silhouette in at least
--votes frames. The result is the largest shape consistent with the outlines. It's the
measurement step: its cross-sections guide a clean hand-built model, and reprojecting it shows
the best fit any single rigid shape can reach against these (slightly inconsistent, pixel-rounded)
frames. Also back-projects the original's colours onto the hull's visible surface to locate colour
regions and lights in 3D.

Writes carve.npz (grid and occupancy), carve-report.md (reprojection fit per vote threshold),
and views of the hull and its colours. Needs numpy, scipy and Pillow.
"""

import argparse
import json
import os

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

import fit_metric
from sprite_geometry import Camera, Sprite, project, silhouette


def grid(bounds, step):
    (x0, x1), (y0, y1), (z0, z1) = bounds
    xs, ys, zs = np.arange(x0, x1, step), np.arange(y0, y1, step), np.arange(z0, z1, step)
    gx, gy, gz = np.meshgrid(xs, ys, zs, indexing="ij")
    return (xs, ys, zs), np.stack([gx.ravel(), gy.ravel(), gz.ravel()], axis=1)


def votes(points, sprite, cam):
    """For each point, in how many frames it falls inside the original silhouette."""
    count = np.zeros(len(points), np.int16)
    for i in range(sprite.n):
        m = sprite.frame_mask(i)
        fx, fy, _ = project(points, sprite, i, cam)
        ix, iy = np.floor(fx).astype(int), np.floor(fy).astype(int)
        ok = (ix >= 0) & (ix < m.shape[1]) & (iy >= 0) & (iy < m.shape[0])
        inside = np.zeros(len(points), bool)
        inside[ok] = m[iy[ok], ix[ok]]
        count += inside
    return count


def reproject(points, sprite, cam):
    masks = [silhouette(points, sprite, i, cam)[0] for i in range(sprite.n)]
    rows = [fit_metric.compare(sprite.frame_mask(i), masks[i]) for i in range(sprite.n)]
    return rows, {
        "iou_mean": float(np.mean([r["iou"] for r in rows])),
        "iou_worst": float(min(r["iou"] for r in rows)),
        "edge_mean": float(np.mean([r["edge_mean"] for r in rows])),
        "edge_p95": float(max(r["edge_p95"] for r in rows)),
    }


def visible_colours(points, normals_ok, sprite, cam, which="image"):
    """Average colour each surface point shows in the frames where it's visible (nearest to the
    camera in its pixel). Returns (mean RGBA per point, number of frames it was seen in)."""
    acc = np.zeros((len(points), 4))
    seen = np.zeros(len(points), int)
    for i in range(sprite.n):
        pix = sprite.frame_pixels(i, which).astype(float)
        fx, fy, depth = project(points, sprite, i, cam)
        ix, iy = np.floor(fx).astype(int), np.floor(fy).astype(int)
        ok = (ix >= 0) & (ix < pix.shape[1]) & (iy >= 0) & (iy < pix.shape[0]) & normals_ok
        idx = np.nonzero(ok)[0]
        key = iy[idx] * pix.shape[1] + ix[idx]
        # z-buffer: keep the nearest point per pixel (and anything within 0.6 units of it)
        order = np.lexsort((depth[idx], key))
        idx, key = idx[order], key[order]
        first = np.ones(len(idx), bool)
        first[1:] = key[1:] != key[:-1]
        nearest = np.maximum.accumulate(np.where(first, np.arange(len(idx)), 0))
        front = depth[idx] <= depth[idx[nearest]] + 0.6
        vis = idx[front]
        acc[vis] += pix[iy[vis], ix[vis]]
        seen[vis] += 1
    return acc / np.maximum(seen, 1)[:, None], seen


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("--sprite", required=True)
    p.add_argument("--data", default="data")
    p.add_argument("--tilt", type=float, required=True)
    p.add_argument("--distance", type=float, default=None, help="perspective distance (default: ortho)")
    p.add_argument("--offset", default="0,0", help="screen offset x,y in 1x px")
    p.add_argument("--step", type=float, default=0.25)
    p.add_argument("--bounds", default="-14,14,-34,22,-18,10", help="x0,x1,y0,y1,z0,z1 model units")
    p.add_argument("--votes", type=int, default=0, help="frames a point must fall inside (default: N-1)")
    p.add_argument("--out", default="out/sprite-render/carve")
    a = p.parse_args()
    os.makedirs(a.out, exist_ok=True)

    sprite = Sprite(a.data, a.sprite)
    cam = Camera(a.tilt, a.distance, 1.0, tuple(float(v) for v in a.offset.split(",")))
    b = [float(v) for v in a.bounds.split(",")]
    axes, pts = grid([(b[0], b[1]), (b[2], b[3]), (b[4], b[5])], a.step)
    count = votes(pts, sprite, cam)

    # Reprojection fit for a range of vote thresholds: how consistent are the frames?
    lines = [f"# Visual hull of {a.sprite}", "",
             f"Camera: tilt {a.tilt:g}, {'perspective ' + format(a.distance, 'g') if a.distance else 'orthographic'},"
             f" offset {a.offset}; grid step {a.step}", "",
             "| Votes needed | Voxels | IoU mean | IoU worst | Edge mean px | Edge p95 px |",
             "|---:|---:|---:|---:|---:|---:|"]
    best = None
    for v in range(sprite.n, sprite.n - 5, -1):
        occ = count >= v
        if not occ.any():
            continue
        rows, s = reproject(pts[occ], sprite, cam)
        lines.append(f"| {v} | {occ.sum()} | {s['iou_mean']:.3f} | {s['iou_worst']:.3f}"
                     f" | {s['edge_mean']:.2f} | {s['edge_p95']:.1f} |")
        if best is None or s["iou_mean"] > best[1]["iou_mean"]:
            best = (v, s, rows)
    votes_needed = a.votes or best[0]
    occ3 = (count >= votes_needed).reshape(len(axes[0]), len(axes[1]), len(axes[2]))
    # keep the largest connected piece (drops stray specks)
    lab, n = ndimage.label(occ3)
    if n > 1:
        sizes = ndimage.sum(occ3, lab, range(1, n + 1))
        occ3 = lab == (1 + int(np.argmax(sizes)))
    lines += ["", f"Chosen: {votes_needed} votes; best reprojection mean IoU {best[1]['iou_mean']:.3f}"]
    with open(os.path.join(a.out, "carve-report.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print("\n".join(lines))

    # Surface voxels and their outward normals (from the smoothed occupancy gradient).
    surf = occ3 & ~ndimage.binary_erosion(occ3)
    smooth = ndimage.gaussian_filter(occ3.astype(float), 1.0)
    gx, gy, gz = np.gradient(smooth)
    idx = np.nonzero(surf)
    spts = np.stack([axes[0][idx[0]], axes[1][idx[1]], axes[2][idx[2]]], axis=1)
    normals = -np.stack([gx[idx], gy[idx], gz[idx]], axis=1)
    normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-9)
    col, seen = visible_colours(spts, np.ones(len(spts), bool), sprite, cam, "image")
    ov, _ = visible_colours(spts, np.ones(len(spts), bool), sprite, cam, "overlay")
    np.savez_compressed(os.path.join(a.out, "carve.npz"), xs=axes[0], ys=axes[1], zs=axes[2],
                        occ=occ3, surf_pts=spts, surf_normals=normals, surf_rgba=col,
                        surf_seen=seen, surf_overlay=ov)

    # Views: occupancy projected along each axis, and colour seen from above and the side.
    def view(arr, axis):
        img = arr.any(axis=axis).astype(np.uint8) * 255
        return Image.fromarray(img.T[::-1] if axis != 2 else img.T[::-1])

    Z = 4
    top = Image.fromarray((occ3.any(axis=2).T[::-1] * 255).astype(np.uint8))   # x right, y up
    side = Image.fromarray((occ3.any(axis=0).T[::-1] * 255).astype(np.uint8))  # y right, z up
    front = Image.fromarray((occ3.any(axis=1).T[::-1] * 255).astype(np.uint8))  # x right, z up
    sheet = Image.new("L", ((top.width + side.width + front.width) * Z + 40, max(top.height, side.height) * Z), 20)
    x = 0
    for im in (top, side, front):
        sheet.paste(im.resize((im.width * Z, im.height * Z), Image.NEAREST), (x, 0))
        x += im.width * Z + 20
    sheet.save(os.path.join(a.out, "hull-views.png"))
    with open(os.path.join(a.out, "carve.json"), "w") as f:
        json.dump({"votes": votes_needed, "summary": best[1], "voxels": int(occ3.sum())}, f, indent=2)


if __name__ == "__main__":
    main()
