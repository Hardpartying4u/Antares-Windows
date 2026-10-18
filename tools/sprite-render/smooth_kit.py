# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Parametric ship shape built from rounded solids (superellipsoids) at any yaw.

For rounded or radial ships the box kit can't express: hubs, pods on angled struts, ellipsoid
hulls. Each part is a superellipsoid

    (|u/a|^p + |v/b|^p)^(q/p) + |w/c|^q <= 1

in its own frame: centred at (x, y, z), turned by `yaw` degrees about the vertical (0 = long
axis b along the nose, +Y), half-sizes a (across), b (along), c (up). p shapes the plan view (2 =
round, high = boxy), q the side view. `mirror=True` adds the copy at -x with -yaw.

    from smooth_kit import SmoothKit, solid
    SHAPE = SmoothKit([solid("hub", x=0, y=2, z=0, a=6, b=7, c=3, yaw=0, p=2.4, q=2.2), ...],
                      look={...})
    globals().update(SHAPE.exports())

Parameters are <name>_x, _y, _z, _a, _b, _c, _yaw, _p, _q, each with search bounds; `fixed`
names parameters to hold (for example a yaw read off the carve). `region` names the colour
region (hull, pods, block, ...). Model units are original 1x pixels. Needs numpy only.
"""

import math

import numpy as np


def solid(name, region="hull", mirror=False, fixed=(), spread=None, **v):
    """A part with starting values; `spread` widens the default search bounds per parameter."""
    spread = spread or {}
    defaults = {"x": 2.0, "y": 2.5, "z": 2.5, "a": 0.4 * v.get("a", 1) + 1, "b": 0.4 * v.get("b", 1) + 1,
                "c": 0.6 * v.get("c", 1) + 1, "yaw": 15.0}
    params = {}
    for key in ("x", "y", "z", "a", "b", "c", "yaw", "p", "q"):
        val = float(v.get(key, {"yaw": 0.0, "p": 2.0, "q": 2.0}.get(key, 0.0)))
        if key in fixed:
            lo = hi = val
        elif key in ("p", "q"):
            lo, hi = 1.6, 10.0
        elif key in ("a", "b", "c"):
            s = spread.get(key, defaults[key])
            lo, hi = max(0.3, val - s), val + s
        else:
            s = spread.get(key, defaults[key])
            lo, hi = val - s, val + s
        params[f"{name}_{key}"] = (val, lo, hi)
    return {"name": name, "region": region, "mirror": mirror, "params": params}


class SmoothKit:
    def __init__(self, parts, look):
        self.parts = parts
        self.look = look
        self.params = {k: v for pt in parts for k, v in pt["params"].items()}
        self.names = [k for k, v in self.params.items() if v[1] != v[2]]  # fitted (not fixed)

    def initial(self):
        return {k: v[0] for k, v in self.params.items()}

    def placements(self, pt, p):
        """(centre x, y, z, yaw degrees) of each copy of a part."""
        n = pt["name"]
        x, y, z, yaw = p[f"{n}_x"], p[f"{n}_y"], p[f"{n}_z"], p[f"{n}_yaw"]
        out = [(x, y, z, yaw)]
        if pt["mirror"]:
            out.append((-x, y, z, -yaw))
        return out

    def part_mask(self, pt, p, x, y, z, grow=0.0):
        n = pt["name"]
        a, b, c = p[f"{n}_a"] + grow, p[f"{n}_b"] + grow, p[f"{n}_c"] + grow
        pe, qe = p[f"{n}_p"], p[f"{n}_q"]
        out = np.zeros(np.broadcast(x, y, z).shape, bool)
        for cx, cy, cz, yaw in self.placements(pt, p):
            t = math.radians(yaw)
            dx, dy = x - cx, y - cy
            u = dx * math.cos(t) + dy * math.sin(t)  # across the part
            v = -dx * math.sin(t) + dy * math.cos(t)  # along the part
            w = z - cz
            with np.errstate(over="ignore"):
                plan = (np.abs(u / a) ** pe + np.abs(v / b) ** pe) ** (qe / pe)
                out = out | (plan + np.abs(w / c) ** qe <= 1.0)
        return out

    def occupancy(self, p, x, y, z):
        out = np.zeros(np.broadcast(x, y, z).shape, bool)
        for pt in self.parts:
            out = out | self.part_mask(pt, p, x, y, z)
        return out

    def roughness_penalty(self, p):
        return 0.0

    def part_of(self, p, pts, tol=0.6):
        """Colour region of each surface point (later parts win, so list small parts last)."""
        x, y, z = pts[:, 0], pts[:, 1], pts[:, 2]
        label = np.full(len(pts), self.parts[0]["region"], dtype=object)
        for pt in self.parts:
            label[self.part_mask(pt, p, x, y, z, grow=tol)] = pt["region"]
        return label

    def exports(self):
        return {"PARAMS": self.params, "NAMES": self.names, "LOOK": self.look, "PARTS": self.parts,
                "SHAPE_KIT": self, "initial": self.initial, "occupancy": self.occupancy,
                "roughness_penalty": self.roughness_penalty, "part_of": self.part_of}
