# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Remastered box-kit ship (box_kit.py: rounded-box parts) for render_sprite.py.

    blender -b --factory-startup -P tools/sprite-render/render_sprite.py -- --sprite uns/carrier \
        --model tools/sprite-render/box_kit_model.py --variant uns_carrier --engine CYCLES

Like twin_pod_model.py, the variant names <variant>_shape.py, <variant>_fit.json and
<variant>_colours.json. Every part becomes a bevelled rounded box exactly as fitted (repeated
parts as separate segments, chamfered fronts sloped). Each part takes its region's material; the
part named by look["strip_on"] carries the measured deck markings. Engine lights sit on the back
of each part in the "pods" region. Materials, lights and camera settings are shared with
twin_pod_model.py.
"""

import importlib
import math
import os
import sys

import bmesh
import bpy
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from twin_pod_model import (CRUISER_LENGTH, DEFAULT_COLOURS, _object,  # noqa: E402
                            _superellipse_ring, glow, load_json, metal, render_settings)

MATERIAL = {  # region: (roughness, roughness variation, plate-line strength, metalness)
    "hull": (0.36, 0.12, 0.35, 0.75), "modules": (0.42, 0.14, 0.3, 0.7),
    "pods": (0.5, 0.15, 0.3, 0.75), "block": (0.5, 0.12, 0.0, 0.35),
}


def box(name, kit, pt, p, cx, y0, y1, mat, bevel):
    """One rounded box along Y, its top sloped by the part's front chamfer if it has one."""
    n = pt["name"]
    bm = bmesh.new()
    seg = 64
    x, z = _superellipse_ring(p[f"{n}_hw"], p[f"{n}_hh"], p[f"{n}_hh"], p[f"{n}_z"], p[f"{n}_exp"], seg)
    c = p.get(f"{n}_chamfer", 0.0)
    ys = [y0, y1]
    if c > 0 and y1 - c > y0:
        ys = [y0] + list(np.linspace(y1 - c, y1, 6))
    rings = []
    for y in ys:
        top = kit.top(pt, p, np.array(y))
        rings.append([bm.verts.new((float(cx + x[j]), float(y), float(min(z[j], top)))) for j in range(seg)])
    for a, b in zip(rings, rings[1:]):
        for j in range(seg):
            bm.faces.new((a[j], b[j], b[(j + 1) % seg], a[(j + 1) % seg]))
    bm.faces.new(rings[0])
    bm.faces.new(list(reversed(rings[-1])))
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-4)
    return _object(name, bm, mat, bevel=bevel)


def build(variant):
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    shape = importlib.import_module(variant + "_shape")
    kit = shape.SHAPE_KIT
    fit = load_json(variant + "_fit.json")
    p, look = fit["params"], shape.LOOK
    colours = {**DEFAULT_COLOURS, "modules": DEFAULT_COLOURS["hull"],
               **(load_json(variant + "_colours.json", {}) or {})}
    front = max(p[f"{pt['name']}_front"] for pt in kit.parts)
    rear = min(p[f"{pt['name']}_rear"] for pt in kit.parts)
    size = (front - rear) / CRUISER_LENGTH
    bevels = look.get("bevel", {})

    mats = {}

    def material(region, markings=None):
        key = (region, bool(markings))
        if key not in mats:
            r, rv, plates, metallic = MATERIAL.get(region, MATERIAL["hull"])
            mats[key] = metal(region + ("_marked" if markings else ""), colours[region], size,
                              rough=r, rough_var=rv, plates=plates, metallic=metallic, markings=markings)
        return mats[key]

    red = glow("engine_light", (1.0, 0.04, 0.02), 3.2)
    for pt in kit.parts:
        n = pt["name"]
        markings = None
        if look.get("strip_on") == n and "strip" in look:
            markings = {"top_z": p[f"{n}_z"] + 0.3 * p[f"{n}_hh"],
                        "strip": {**look["strip"], "colour": colours["strip"]}}
        mat = material(pt["region"], markings)
        bevel = bevels.get(n, bevels.get("default", 0.3 * size))
        sides = (-1, 1) if pt["mirror"] else (1,)
        for side in sides:
            cx = side * p[f"{n}_x"]
            for k, (y0, y1) in enumerate(kit.segments(pt, p)):
                box(f"{n}{side:+d}{k}", kit, pt, p, cx, y0, y1, mat, bevel)
            if pt["region"] == "pods" and look.get("lights") == "per_pod":
                r = min(0.35 * p[f"{n}_hw"], 0.35 * p[f"{n}_hh"], 1.6)
                bpy.ops.mesh.primitive_cylinder_add(
                    location=(cx, p[f"{n}_rear"] - 0.06 * size, p[f"{n}_z"]), radius=r,
                    depth=0.12 * size, rotation=(math.pi / 2, 0, 0), vertices=24)
                bpy.context.active_object.data.materials.append(red)

    return render_settings(fit)
