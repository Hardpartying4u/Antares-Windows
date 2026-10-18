# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Projection of 3D model points into an Antares sprite's original frames (numpy only).

Matches render_sprite.py exactly: the model is turned clockwise by 360/N degrees per frame about
its pivot (the origin, nose along +Y), and seen by a camera tilted `tilt` degrees from vertical
(from the bottom of the screen), either orthographic or perspective at `distance` units from the
pivot. At the pivot, one model unit is `scale` pixels, and the pivot lands on the frame's
(cx, cy) plus `offset`.
"""

import math
import os
import re

import numpy as np
from PIL import Image

FRAME_RE = re.compile(
    r"left:\s*(-?\d+),\s*top:\s*(-?\d+),\s*right:\s*(-?\d+),\s*bottom:\s*(-?\d+),"
    r"\s*cx:\s*(-?\d+),\s*cy:\s*(-?\d+)")


class Camera:
    def __init__(self, tilt, distance=None, scale=1.0, offset=(0.0, 0.0)):
        self.tilt, self.distance, self.scale, self.offset = tilt, distance, scale, offset
        t = math.radians(tilt)
        # world -> camera axes for a camera rotated by +tilt about X (Blender: x right, y up,
        # looking down -Z)
        self.r_t = np.array([[1, 0, 0], [0, math.cos(t), math.sin(t)], [0, -math.sin(t), math.cos(t)]])


class Sprite:
    def __init__(self, data, name):
        with open(os.path.join(data, "sprites", name + ".pn"), encoding="utf-8") as f:
            self.frames = [tuple(map(int, m)) for m in FRAME_RE.findall(f.read())]
        self.image = np.asarray(Image.open(os.path.join(data, "sprites", name, "image.png")).convert("RGBA"))
        self.overlay = np.asarray(Image.open(os.path.join(data, "sprites", name, "overlay.png")).convert("RGBA"))
        self.alpha = self.image[..., 3] >= 128
        self.n = len(self.frames)

    def frame_mask(self, i):
        l, t, r, b = self.frames[i][:4]
        return self.alpha[t:b, l:r]

    def frame_pixels(self, i, which="image"):
        l, t, r, b = self.frames[i][:4]
        return (self.image if which == "image" else self.overlay)[t:b, l:r]


def project(points, sprite, i, cam):
    """Projects Nx3 model points into frame i. Returns (fx, fy, depth): frame pixel coordinates
    (x right, y down, pixel centres at +0.5) and distance from the camera (larger = farther)."""
    l, t, r, b, cx, cy = sprite.frames[i]
    w, h, px, py = r - l, b - t, cx - l, cy - t
    a = -2 * math.pi * i / sprite.n  # clockwise from above
    rz = np.array([[math.cos(a), -math.sin(a), 0], [math.sin(a), math.cos(a), 0], [0, 0, 1]])
    q = points @ (cam.r_t @ rz).T
    s = cam.scale
    ox = (px + cam.offset[0] - w / 2) / s
    oy = (h / 2 - py - cam.offset[1]) / s
    if cam.distance:
        depth = cam.distance - q[:, 2]
        k = cam.distance / depth
    else:
        depth = -q[:, 2]
        k = 1.0
    fx = w / 2 + s * (q[:, 0] + ox) * k
    fy = h / 2 - s * (q[:, 1] + oy) * k
    return fx, fy, depth


def silhouette(points, sprite, i, cam):
    """Boolean frame mask of the pixels the points fall in (points must be dense: < 1 px apart)."""
    l, t, r, b = sprite.frames[i][:4]
    w, h = r - l, b - t
    fx, fy, _ = project(points, sprite, i, cam)
    ix, iy = np.floor(fx).astype(int), np.floor(fy).astype(int)
    ok = (ix >= 0) & (ix < w) & (iy >= 0) & (iy < h)
    m = np.zeros((h, w), bool)
    m[iy[ok], ix[ok]] = True
    return m, (~ok).sum() / max(1, len(points))
