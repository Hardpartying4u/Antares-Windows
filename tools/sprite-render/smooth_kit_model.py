# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Remastered smooth-kit ship (smooth_kit.py: rounded solids at any yaw) for render_sprite.py.

    blender -b --factory-startup -P tools/sprite-render/render_sprite.py -- --sprite ish/cruiser \
        --model tools/sprite-render/smooth_kit_model.py --variant ish_cruiser --engine CYCLES

Like the other kit models, the variant names <variant>_shape.py, <variant>_fit.json and
<variant>_colours.json. Every solid becomes a smooth superellipsoid mesh exactly as fitted, with
its region's material (shared with twin_pod_model.py). look["lights"] lists the lights measured
from the original (carve.py): {"x", "y", "z", "r", "mirror", "colour"}; look["glow"] sets their
default colour. Materials, camera and lights come from twin_pod_model.py.
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
from twin_pod_model import (CRUISER_LENGTH, DEFAULT_COLOURS, _object, glow,  # noqa: E402
                            load_json, metal, render_settings)

MATERIAL = {  # region: (roughness, roughness variation, plate-line strength, metalness)
    "hull": (0.36, 0.12, 0.35, 0.75), "pods": (0.45, 0.14, 0.3, 0.75),
    "modules": (0.42, 0.14, 0.3, 0.7), "block": (0.5, 0.12, 0.0, 0.35), "struts": (0.45, 0.1, 0.0, 0.8),
}


def _signed_pow(x, e):
    return np.sign(x) * np.abs(x) ** e


def superellipsoid(name, a, b, c, pe, qe, centre, yaw, mat, segs=48, rings=32):
    """Mesh of (|u/a|^p + |v/b|^p)^(q/p) + |w/c|^q = 1, turned by yaw and moved to centre."""
    bm = bmesh.new()
    eta = np.linspace(-math.pi / 2, math.pi / 2, rings + 1)[1:-1]  # latitude, poles added below
    omega = np.linspace(-math.pi, math.pi, segs, endpoint=False)
    t = math.radians(yaw)
    ct, st = math.cos(t), math.sin(t)

    def place(u, v, w):
        return (centre[0] + u * ct - v * st, centre[1] + u * st + v * ct, centre[2] + w)

    rows = []
    for e in eta:
        ce, se = _signed_pow(math.cos(e), 2 / qe), _signed_pow(math.sin(e), 2 / qe)
        row = []
        for o in omega:
            u = a * ce * _signed_pow(math.cos(o), 2 / pe)
            v = b * ce * _signed_pow(math.sin(o), 2 / pe)
            row.append(bm.verts.new(place(float(u), float(v), float(c * se))))
        rows.append(row)
    bottom = bm.verts.new(place(0.0, 0.0, -c))
    top = bm.verts.new(place(0.0, 0.0, c))
    for r0, r1 in zip(rows, rows[1:]):
        for j in range(segs):
            bm.faces.new((r0[j], r0[(j + 1) % segs], r1[(j + 1) % segs], r1[j]))
    for j in range(segs):
        bm.faces.new((rows[0][(j + 1) % segs], rows[0][j], bottom))
        bm.faces.new((rows[-1][j], rows[-1][(j + 1) % segs], top))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return _object(name, bm, mat)


def build(variant):
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    shape = importlib.import_module(variant + "_shape")
    kit = shape.SHAPE_KIT
    fit = load_json(variant + "_fit.json")
    p, look = fit["params"], shape.LOOK
    colours = {**DEFAULT_COLOURS, "modules": DEFAULT_COLOURS["hull"], "struts": DEFAULT_COLOURS["pods"],
               **(load_json(variant + "_colours.json", {}) or {})}
    ys = [p[f"{pt['name']}_y"] + s * p[f"{pt['name']}_b"] for pt in kit.parts for s in (-1, 1)]
    size = (max(ys) - min(ys)) / CRUISER_LENGTH

    mats = {}
    for pt in kit.parts:
        n, region = pt["name"], pt["region"]
        if region not in mats:
            r, rv, plates, metallic = MATERIAL.get(region, MATERIAL["hull"])
            mats[region] = metal(region, colours.get(region, colours["hull"]), size, rough=r,
                                 rough_var=rv, plates=plates, metallic=metallic)
        for k, (cx, cy, cz, yaw) in enumerate(kit.placements(pt, p)):
            superellipsoid(f"{n}{k}", p[f"{n}_a"], p[f"{n}_b"], p[f"{n}_c"], p[f"{n}_p"], p[f"{n}_q"],
                           (cx, cy, cz), yaw, mats[region])

    lights = {}
    for lt in look.get("lights", []):
        colour = tuple(lt.get("colour", look.get("glow", (1.0, 0.04, 0.02))))
        if colour not in lights:
            lights[colour] = glow(f"light{len(lights)}", colour, 3.2)
        for sx in ((1, -1) if lt.get("mirror") else (1,)):
            bpy.ops.mesh.primitive_uv_sphere_add(location=(sx * lt["x"], lt["y"], lt["z"]),
                                                 radius=lt["r"], segments=24, ring_count=12)
            bpy.context.active_object.data.materials.append(lights[colour])
    return render_settings(fit)
