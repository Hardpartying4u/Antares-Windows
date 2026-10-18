#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Calibrate a remaster's base colours against the original sprite, region by region.

    python tools/sprite-render/calibrate_colours.py --sprite uns/cruiser \
        --render out/sprite-render/remaster --carve out/sprite-render/carve-200 \
        --colours tools/sprite-render/uns_cruiser_colours.json

Back-projects both the original frames and the rendered 1x frames onto the same carved surface
points (carve.py), averages each colour region (hull, dorsal strip, cockpit, pods, block), and
scales each region's linear base colour by original / render, so the remaster has the original's
overall tone per part while keeping its own lighting and materials. Run, re-render, repeat (one or
two rounds converge). Writes the colours JSON and prints a table. Needs numpy, scipy, Pillow.
"""

import argparse
import json
import os

import numpy as np
from PIL import Image

import importlib

from carve import visible_colours
from sprite_geometry import Camera, Sprite

# Colour regions on the cruiser's hull, in model units (measured from the original, carve.py; the
# same numbers mark the regions in uns_cruiser_remaster.py). Twin-pod ships carry theirs in LOOK.
CRUISER_LOOK = {"strip": {"hw": 1.0, "y": (-12.0, 5.5)}, "cockpit": {"hw": 1.75, "y": (6.0, 10.5)}}


def regions(shape, p, pts, normals):
    look = getattr(shape, "LOOK", CRUISER_LOOK)
    part = shape.part_of(p, pts)
    x, y = np.abs(pts[:, 0]), pts[:, 1]
    up = normals[:, 2] > 0.5
    label = part.astype(object)
    label[(part == "pod")] = "pods"
    for key in ("strip", "cockpit"):
        if key in look:
            r = look[key]
            x0, x1 = r.get("x", (-1.0, r.get("hw")))
            label[(part == "hull") & up & (x > x0) & (x < x1) & (y > r["y"][0]) & (y < r["y"][1])] = key
    return label


def srgb_to_linear(c):
    c = c / 255.0
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def main():
    a = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    a.add_argument("--sprite", required=True)
    a.add_argument("--data", default="data")
    a.add_argument("--render", required=True)
    a.add_argument("--carve", required=True)
    a.add_argument("--shape", default="uns_cruiser_shape", help="shape module of the ship")
    a.add_argument("--fit", help="fitted parameters (default: <ship>_fit.json next to the shape)")
    a.add_argument("--colours", required=True, help="JSON of linear base colours to update")
    a.add_argument("--damping", type=float, default=0.8, help="fraction of the correction to apply")
    args = a.parse_args()

    shape = importlib.import_module(args.shape)
    fit_path = args.fit or os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                        args.shape.replace("_shape", "_fit.json"))
    with open(fit_path, encoding="utf-8") as f:
        fit = json.load(f)
    p, cam_cfg = fit["params"], fit["camera"]
    cam = Camera(cam_cfg["tilt"], cam_cfg["distance"])
    d = np.load(os.path.join(args.carve, "carve.npz"))
    pts, normals = d["surf_pts"], d["surf_normals"]

    orig = Sprite(args.data, args.sprite)
    new = Sprite(args.data, args.sprite)  # same frames, rendered pixels
    new.image = np.asarray(Image.open(os.path.join(args.render, "sprites", args.sprite, "image.png")).convert("RGBA"))
    ok = np.ones(len(pts), bool)
    co, seen_o = visible_colours(pts, ok, orig, cam)
    cn, seen_n = visible_colours(pts, ok, new, cam)
    # skip the red lights and anything seen too rarely to trust
    red = (co[:, 0] > co[:, 1] * 1.6) & (co[:, 0] > 90)
    use = (seen_o >= 3) & (seen_n >= 3) & ~red & (co[:, 3] > 200) & (cn[:, 3] > 200)
    label = regions(shape, p, pts, normals)

    with open(args.colours, encoding="utf-8") as f:
        colours = json.load(f)
    print(f"{'region':8} {'points':>6}  {'original sRGB':>15}  {'render sRGB':>15}  new linear base")
    for name in ("hull", "strip", "cockpit", "pods", "modules", "block"):
        m = use & (label == name)
        if m.sum() < 20 or name not in colours:
            continue
        lo = srgb_to_linear(co[m, :3]).mean(0)
        ln = srgb_to_linear(cn[m, :3]).mean(0)
        ratio = np.clip(lo / np.maximum(ln, 1e-4), 0.25, 4.0) ** args.damping
        base = np.clip(np.array(colours[name]) * ratio, 0.003, 0.95)
        colours[name] = [round(float(v), 4) for v in base]
        print(f"{name:8} {m.sum():6d}  {np.array2string(co[m, :3].mean(0), precision=0):>15}"
              f"  {np.array2string(cn[m, :3].mean(0), precision=0):>15}  {colours[name]}")
    with open(args.colours, "w", encoding="utf-8") as f:
        json.dump(colours, f, indent=2)


if __name__ == "__main__":
    main()
