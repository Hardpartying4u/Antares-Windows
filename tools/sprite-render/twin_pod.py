# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Shared parametric shape for ships built as a hull, two engine pods and an engine block.

The same layout as uns_cruiser_shape.py, with the stations, parameter bounds and look of each ship
supplied by a small per-ship module (for example uns_fighter_shape.py):

    from twin_pod import TwinPod
    SHAPE = TwinPod(stations=[...], params={...}, look={...})
    globals().update(SHAPE.exports())

Model units are original 1x pixels, nose along +Y, pivot at the origin. Parts:
  hull   lofted superellipse sections through the stations; half-width rx and half-height rz are
         free at each station; the section centre follows z = zc0 + zslope * y; ends in a point
         at nose_y. Underside depth is rz * hull_bot.
  pods   two mirrored rounded boxes (superellipse cross-section) from pod_rear to pod_front.
  block  the engine block between the pods, a rounded box.

`look` describes surface features measured from the original (carve.py back-projection), used by
twin_pod_model.py and calibrate_colours.py:
  strip    {"hw", "y": (y0, y1)} darker dorsal band, or absent
  cockpit  {"hw", "y": (y0, y1)} dark window on top of the hull, or absent
  groove_z z of a dark groove splitting each pod into an upper and a lower nacelle, or absent
  lights   "per_pod" (one light at the pod's centre height) or "per_nacelle"
  bevel    {"hull", "pod", "block"} bevel widths in model units
Needs numpy only, so Blender's Python can use it.
"""

import numpy as np


def pchip(xk, yk, x):
    """Monotone piecewise-cubic (PCHIP, Fritsch-Carlson) interpolation, matching scipy's
    PchipInterpolator, including its three-point end slopes."""
    xk, yk, x = np.asarray(xk, float), np.asarray(yk, float), np.asarray(x, float)
    h = np.diff(xk)
    d = np.diff(yk) / h
    m = np.zeros_like(yk)
    for k in range(1, len(xk) - 1):
        if d[k - 1] * d[k] > 0:
            w1, w2 = 2 * h[k] + h[k - 1], h[k] + 2 * h[k - 1]
            m[k] = (w1 + w2) / (w1 / d[k - 1] + w2 / d[k])

    def edge(h0, h1, m0, m1):
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


def _superellipse(u, v, e):
    return np.abs(u) ** e + np.abs(v) ** e <= 1.0


class TwinPod:
    def __init__(self, stations, params, look):
        self.stations = np.asarray(stations, float)
        self.rear = float(self.stations[0])  # the hull starts inside the engine block
        self.params = params  # name: (initial, low, high)
        self.look = look

    def initial(self):
        return {k: v[0] for k, v in self.params.items()}

    def hull_profile(self, p, y):
        """Half-width, half-height and centre height of the hull section at y (arrays)."""
        n = len(self.stations)
        ys = np.append(self.stations, p["nose_y"])
        y = np.asarray(y, float)
        rx = pchip(ys, [p[f"rx{k}"] for k in range(n)] + [0.0], y)
        rz = pchip(ys, [p[f"rz{k}"] for k in range(n)] + [0.0], y)
        inside = (y >= self.rear) & (y <= p["nose_y"])
        return (np.where(inside, np.maximum(rx, 0), 0), np.where(inside, np.maximum(rz, 0), 0),
                p["zc0"] + p["zslope"] * y)

    def occupancy(self, p, x, y, z):
        """Boolean: which points (arrays) are inside the ship."""
        rx, rz, zc = self.hull_profile(p, y)
        with np.errstate(divide="ignore", invalid="ignore"):
            rzz = np.where(z < zc, rz * p["hull_bot"], rz)
            hull = (rx > 0) & _superellipse(x / np.where(rx > 0, rx, 1),
                                            (z - zc) / np.where(rzz > 0, rzz, 1), p["hull_exp"])
        pod_y = (y <= p["pod_front"]) & (y >= p["pod_rear"])
        pods = pod_y & _superellipse((np.abs(x) - p["pod_x"]) / p["pod_hw"],
                                     (z - p["pod_z"]) / p["pod_hh"], p["pod_exp"])
        blk_y = (y <= p["blk_front"]) & (y >= p["blk_rear"])
        block = blk_y & _superellipse(x / p["blk_hw"], (z - p["blk_z"]) / p["blk_hh"], p["blk_exp"])
        return hull | pods | block

    def roughness_penalty(self, p):
        """Squared second differences of the station radii: discourages ripples the silhouettes
        can't confirm (all views share one elevation, so heights trade off against widths)."""
        n = len(self.stations)
        pen = 0.0
        for key in ("rx", "rz"):
            v = np.array([p[f"{key}{k}"] for k in range(n)])
            pen += float(np.sum(np.diff(v, 2) ** 2))
        return pen

    def part_of(self, p, pts):
        """Label surface points by part: 'pod', 'block' or 'hull'."""
        x, y = pts[:, 0], pts[:, 1]
        pod = ((y <= p["pod_front"] + 0.5) & (y >= p["pod_rear"] - 0.5)
               & (np.abs(x) > p["pod_x"] - p["pod_hw"] - 0.3))
        blk = (~pod & (y <= p["blk_front"] + 0.5) & (y >= p["blk_rear"] - 0.5)
               & (np.abs(x) <= p["blk_hw"] + 0.3))
        return np.where(pod, "pod", np.where(blk, "block", "hull"))

    def exports(self):
        """Module-level names fit_shape.py and the model script expect."""
        return {"STATIONS": self.stations, "HULL_REAR": self.rear, "PARAMS": self.params,
                "NAMES": list(self.params), "LOOK": self.look, "initial": self.initial,
                "hull_profile": self.hull_profile, "occupancy": self.occupancy,
                "roughness_penalty": self.roughness_penalty, "part_of": self.part_of}
