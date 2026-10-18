# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Parametric shape of the Human (UNS) cruiser, shared by the fitter and the Blender model.

The shape is defined in model units (about one original pixel), nose along +Y, pivot at the
origin. Parts:
  hull   lofted superellipse sections through fixed stations along Y; half-width rx and
         half-height rz are free at each station, and the section centre follows a straight spine
         z = zc0 + zslope * y. Ends in a point at nose_y.
  pods   two mirrored rounded boxes (superellipse cross-section) from pod_rear to pod_front.
  block  the engine block between the pods, a rounded box.

fit_shape.py tunes PARAMS against the original frames; uns_cruiser_remaster.py builds meshes from
the same functions, so what was fitted is what gets rendered.
"""

import numpy as np


def pchip(xk, yk, x):
    """Monotone piecewise-cubic (PCHIP, Fritsch-Carlson) interpolation, numpy only, so Blender's
    Python can use this module too. Extends linearly beyond the ends."""
    xk, yk, x = np.asarray(xk, float), np.asarray(yk, float), np.asarray(x, float)
    h = np.diff(xk)
    d = np.diff(yk) / h
    m = np.zeros_like(yk)
    for k in range(1, len(xk) - 1):
        if d[k - 1] * d[k] > 0:
            w1, w2 = 2 * h[k] + h[k - 1], h[k] + 2 * h[k - 1]
            m[k] = (w1 + w2) / (w1 / d[k - 1] + w2 / d[k])
    def edge(h0, h1, m0, m1):  # scipy's three-point end slopes
        e = ((2 * h0 + h1) * m0 - h0 * m1) / (h0 + h1)
        if np.sign(e) != np.sign(m0):
            return 0.0
        if np.sign(m0) != np.sign(m1) and abs(e) > abs(3 * m0):
            return 3 * m0
        return e
    m[0] = edge(h[0], h[1], d[0], d[1])
    m[-1] = edge(h[-1], h[-2], d[-1], d[-2])
    i = np.clip(np.searchsorted(xk, x) - 1, 0, len(h) - 1)
    t = (x - xk[i]) / h[i]
    h00, h10, h01, h11 = 2 * t**3 - 3 * t**2 + 1, t**3 - 2 * t**2 + t, -2 * t**3 + 3 * t**2, t**3 - t**2
    return h00 * yk[i] + h10 * h[i] * m[i] + h01 * yk[i + 1] + h11 * h[i] * m[i + 1]

STATIONS = np.array([-16.0, -12.0, -9.0, -6.0, -3.0, 0.0, 3.0, 6.0, 9.0, 12.0])
HULL_REAR = -16.0  # inside the engine block

# name: (initial, low, high). Initial values come from the carved visual hull (carve.py).
PARAMS = {
    **{f"rx{k}": (v, 1.0, 8.0) for k, v in enumerate([4.2, 4.2, 4.6, 4.9, 5.5, 6.2, 5.0, 4.2, 3.2, 2.3])},
    **{f"rz{k}": (v, 0.8, 7.0) for k, v in enumerate([3.0, 3.0, 3.3, 3.5, 3.9, 4.3, 3.6, 3.0, 2.4, 1.8])},
    "zc0": (-4.5, -9.0, 0.0), "zslope": (0.0, -0.2, 0.2), "hull_exp": (2.2, 1.6, 4.0),
    "hull_bot": (1.0, 0.5, 1.8),  # underside depth relative to the top
    "nose_y": (14.6, 12.0, 16.5),
    "pod_x": (6.5, 4.0, 9.0), "pod_hw": (2.0, 1.0, 4.0), "pod_hh": (2.8, 1.0, 7.5),
    "pod_z": (-4.0, -9.0, 0.0), "pod_front": (-14.0, -18.0, -10.0), "pod_rear": (-25.6, -28.0, -23.0),
    "pod_exp": (4.0, 2.2, 10.0),
    "blk_hw": (2.8, 1.0, 4.5), "blk_hh": (2.2, 1.0, 5.0), "blk_z": (-4.5, -9.0, 0.0),
    "blk_front": (-14.0, -18.0, -10.0), "blk_rear": (-23.8, -27.0, -20.0), "blk_exp": (4.0, 2.2, 10.0),
}
NAMES = list(PARAMS)


def initial():
    return {k: v[0] for k, v in PARAMS.items()}


def hull_profile(p, y):
    """Half-width, half-height and centre height of the hull section at y (arrays)."""
    ys = np.append(STATIONS, p["nose_y"])
    y = np.asarray(y, float)
    rx = pchip(ys, [p[f"rx{k}"] for k in range(len(STATIONS))] + [0.0], y)
    rz = pchip(ys, [p[f"rz{k}"] for k in range(len(STATIONS))] + [0.0], y)
    inside = (y >= HULL_REAR) & (y <= p["nose_y"])
    return np.where(inside, np.maximum(rx, 0), 0), np.where(inside, np.maximum(rz, 0), 0), \
        p["zc0"] + p["zslope"] * y


def _superellipse(u, v, e):
    return np.abs(u) ** e + np.abs(v) ** e <= 1.0


def occupancy(p, x, y, z):
    """Boolean: which points (arrays) are inside the ship."""
    rx, rz, zc = hull_profile(p, y)
    with np.errstate(divide="ignore", invalid="ignore"):
        rzz = np.where(z < zc, rz * p["hull_bot"], rz)  # deeper or shallower underside
        hull = (rx > 0) & _superellipse(x / np.where(rx > 0, rx, 1), (z - zc) / np.where(rzz > 0, rzz, 1),
                                        p["hull_exp"])
    pod_y = (y <= p["pod_front"]) & (y >= p["pod_rear"])
    pods = pod_y & _superellipse((np.abs(x) - p["pod_x"]) / p["pod_hw"], (z - p["pod_z"]) / p["pod_hh"],
                                 p["pod_exp"])
    blk_y = (y <= p["blk_front"]) & (y >= p["blk_rear"])
    block = blk_y & _superellipse(x / p["blk_hw"], (z - p["blk_z"]) / p["blk_hh"], p["blk_exp"])
    return hull | pods | block


def roughness_penalty(p):
    """Sum of squared second differences of the station radii: discourages ripples that the
    silhouettes can't confirm (the views all share one elevation, so section heights trade
    off against widths)."""
    pen = 0.0
    for key in ("rx", "rz"):
        v = np.array([p[f"{key}{k}"] for k in range(len(STATIONS))])
        pen += float(np.sum(np.diff(v, 2) ** 2))
    return pen


def part_of(p, pts):
    """Label each surface point by part: 'pod', 'block', 'hull' (for colour regions)."""
    x, y, z = pts[:, 0], pts[:, 1], pts[:, 2]
    pod = (y <= p["pod_front"] + 0.5) & (y >= p["pod_rear"] - 0.5) & (np.abs(x) > p["pod_x"] - p["pod_hw"] - 0.3)
    blk = ~pod & (y <= p["blk_front"] + 0.5) & (y >= p["blk_rear"] - 0.5) & (np.abs(x) <= p["blk_hw"] + 0.3)
    return np.where(pod, "pod", np.where(blk, "block", "hull"))
