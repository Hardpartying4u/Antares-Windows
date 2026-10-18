# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Parametric ship shape built from rounded boxes, for ships that aren't a twin-pod layout.

A ship module lists its parts; each is a superellipse-section prism along Y (nose = +Y), fitted to
the original frames by fit_shape.py:

    from box_kit import BoxKit, part
    SHAPE = BoxKit([part("deck", x=0, hw=8, hh=2.5, z=13, front=45, rear=-22, exp=6, chamfer=6),
                    part("pod", mirror=True, x=10.8, hw=4, ...), ...], look={...})
    globals().update(SHAPE.exports())

Per part, parameters <name>_x, _hw, _hh, _z, _front, _rear, _exp and optionally _round (the front
end rounds off in plan view over this length), _chamfer (the top
slopes down over this length at the front) and _gap (`repeat` > 1: equal segments separated by
gaps, such as hangar modules). Each starting value comes with search bounds. `region` names the
colour region the part belongs to (hull, pods, block, ...), used by box_kit_model.py and
calibrate_colours.py. Model units are original 1x pixels. Needs numpy only.
"""

import numpy as np

CHAMFER_DROP = 0.6  # a chamfer lowers the top by this share of the full height at the front


def part(name, region="hull", mirror=False, repeat=1, spread=None, **values):
    """A part with starting values. `spread` widens the default search bounds."""
    spread = spread or {}
    bounds = {}
    for key, v in values.items():
        s = spread.get(key, {"x": 2.0, "hw": 0.4 * v + 1, "hh": 0.6 * v + 1, "z": 4.0, "front": 4.0,
                             "rear": 4.0, "exp": None, "chamfer": None, "gap": None}.get(key))
        if key == "exp":
            lo, hi = 2.2, 10.0
        elif key in ("chamfer", "round"):
            lo, hi = 0.0, max(1.0, 2.5 * v)
        elif key == "gap":
            lo, hi = 0.0, max(0.5, 3 * v)
        elif key in ("hw", "hh"):
            lo, hi = max(0.3, v - s), v + s
        else:
            lo, hi = v - s, v + s
        bounds[f"{name}_{key}"] = (float(v), float(lo), float(hi))
    return {"name": name, "region": region, "mirror": mirror, "repeat": repeat, "params": bounds}


def _superellipse(u, v, e):
    return np.abs(u) ** e + np.abs(v) ** e <= 1.0


class BoxKit:
    def __init__(self, parts, look):
        self.parts = parts
        self.look = look
        self.params = {k: v for pt in parts for k, v in pt["params"].items()}

    def initial(self):
        return {k: v[0] for k, v in self.params.items()}

    def segments(self, pt, p):
        """(rear, front) of each segment of a part."""
        n = pt["name"]
        rear, front = p[f"{n}_rear"], p[f"{n}_front"]
        if pt["repeat"] <= 1:
            return [(rear, front)]
        gap = p[f"{n}_gap"]
        length = (front - rear - gap * (pt["repeat"] - 1)) / pt["repeat"]
        return [(rear + k * (length + gap), rear + k * (length + gap) + length) for k in range(pt["repeat"])]

    def top(self, pt, p, y):
        """Height of the part's top at y (lowered by the front chamfer)."""
        n = pt["name"]
        t = p[f"{n}_z"] + p[f"{n}_hh"]
        c = p.get(f"{n}_chamfer", 0.0)
        if c > 0:
            into = np.clip((y - (p[f"{n}_front"] - c)) / c, 0, 1)
            t = t - into * CHAMFER_DROP * 2 * p[f"{n}_hh"]
        return t

    def half_width(self, pt, p, y):
        """Half-width at y: narrowed towards the front by the plan-view rounding, if any."""
        n = pt["name"]
        r = p.get(f"{n}_round", 0.0)
        if r <= 0:
            return p[f"{n}_hw"]
        into = np.clip((y - (p[f"{n}_front"] - r)) / r, 0, 1)
        return p[f"{n}_hw"] * np.maximum(np.sqrt(1 - into ** 2), 0.15)

    def part_mask(self, pt, p, x, y, z):
        n = pt["name"]
        ax = np.abs(x) if pt["mirror"] else x
        m = _superellipse((ax - p[f"{n}_x"]) / self.half_width(pt, p, y), (z - p[f"{n}_z"]) / p[f"{n}_hh"],
                          p[f"{n}_exp"])
        iny = np.zeros(np.broadcast(x, y, z).shape, bool)
        for r, f in self.segments(pt, p):
            iny = iny | ((y >= r) & (y <= f))
        m = m & iny
        if f"{n}_chamfer" in p:
            m = m & (z <= self.top(pt, p, y))
        return m

    def occupancy(self, p, x, y, z):
        out = np.zeros(np.broadcast(x, y, z).shape, bool)
        for pt in self.parts:
            out = out | self.part_mask(pt, p, x, y, z)
        return out

    def roughness_penalty(self, p):
        return 0.0

    def part_of(self, p, pts, tol=0.6):
        """Colour region of each surface point: the region of the nearest part containing it
        (points are on the surface, so test a slightly grown part). With look["label_plan"]
        (nearly overhead cameras, where the carved hull's heights are meaningless) a point takes
        the region of the topmost part covering it in plan view."""
        x, y, z = pts[:, 0], pts[:, 1], pts[:, 2]
        if self.look.get("label_plan"):
            label = np.full(len(pts), "hull", dtype=object)
            best = np.full(len(pts), -np.inf)
            for pt in self.parts:
                n = pt["name"]
                ax = np.abs(x) if pt["mirror"] else x
                inside = np.abs(ax - p[f"{n}_x"]) <= self.half_width(pt, p, y) + tol
                iny = np.zeros(len(pts), bool)
                for r, f in self.segments(pt, p):
                    iny |= (y >= r - tol) & (y <= f + tol)
                top = np.broadcast_to(self.top(pt, p, y), y.shape)
                win = inside & iny & (top > best)
                label[win], best[win] = pt["region"], top[win]
            return label
        label = np.full(len(pts), self.parts[0]["region"], dtype=object)
        grown = dict(p)
        for pt in self.parts:
            n = pt["name"]
            grown[f"{n}_hw"] = p[f"{n}_hw"] + tol
            grown[f"{n}_hh"] = p[f"{n}_hh"] + tol
            grown[f"{n}_front"] = p[f"{n}_front"] + tol
            grown[f"{n}_rear"] = p[f"{n}_rear"] - tol
        for pt in self.parts:  # later parts win, so list small parts after big ones
            label[self.part_mask(pt, grown, x, y, z)] = pt["region"]
        return label

    def exports(self):
        return {"PARAMS": self.params, "NAMES": list(self.params), "LOOK": self.look,
                "PARTS": self.parts, "SHAPE_KIT": self, "initial": self.initial,
                "occupancy": self.occupancy, "roughness_penalty": self.roughness_penalty,
                "part_of": self.part_of}
