# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Remastered twin-pod ship (hull, two engine pods, engine block) for render_sprite.py.

    blender -b --factory-startup -P tools/sprite-render/render_sprite.py -- --sprite uns/fighter \
        --model tools/sprite-render/twin_pod_model.py --variant uns_fighter --engine CYCLES

The variant names the ship: <variant>_shape.py (twin_pod.py layout, stations, bounds and `look`),
<variant>_fit.json (parameters fitted to the original frames by fit_shape.py) and
<variant>_colours.json (base colours calibrated against the original by calibrate_colours.py).
Builds clean meshes from exactly the fitted shape: a smooth lofted hull, pods and block as rounded
boxes, bevelled edges. Surface features come only from `look`, which records what the original
shows: dorsal strip, cockpit window, pods split into nacelles, engine lights. Every part takes the
team colour, lights included, as in the originals (docs/art-direction.md section 3).
This is the generic form of uns_cruiser_remaster.py.
"""

import importlib
import json
import math
import os
import sys

import bmesh
import bpy
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

DEFAULT_COLOURS = {  # linear RGB; calibrate_colours.py writes the measured ones
    "hull": [0.42, 0.43, 0.45], "strip": [0.09, 0.092, 0.09], "pods": [0.32, 0.325, 0.34],
    "block": [0.03, 0.03, 0.028], "cockpit": [0.007, 0.007, 0.006],
}
CRUISER_LENGTH = 41.0  # nose to pod rear of uns/cruiser, the scale the material settings were tuned at


def load_json(name, default=None):
    path = os.path.join(HERE, name)
    if os.path.exists(path):
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    return default


def render_settings(fit):
    """Camera and lights for render_sprite.py, shared by the remaster model scripts."""
    return {
        "tilt": fit["camera"]["tilt"],
        "camera_distance": fit["camera"]["distance"],
        "scale": 1.0,  # shapes are fitted in original-pixel units with this camera
        # Sub-pixel placement differs per ship (half a pixel either way), so it is measured on a
        # first render and stored in the fit file: fit_metric.py --set-offset.
        "screen_offset": tuple(fit["camera"].get("screen_offset", (0.0, 0.0))),
        "ambient": 0.05,
        "lights": [
            {"name": "key", "azimuth": 315, "elevation": 48, "energy": 3.6,
             "color": (1.0, 0.97, 0.92), "angle": 6.0},
            {"name": "rim", "azimuth": 135, "elevation": -12, "energy": 1.1,
             "color": (0.72, 0.84, 1.0), "angle": 3.0},
            {"name": "fill", "azimuth": 200, "elevation": 70, "energy": 0.5,
             "color": (0.85, 0.9, 1.0), "angle": 30.0},
        ],
    }


# ---------------------------------------------------------------------------------------------
# Materials


def _mix_rgb(nt, blend, fac, a, b=None, b_value=None):
    n = nt.nodes.new("ShaderNodeMix")
    n.data_type, n.blend_type = "RGBA", blend
    if isinstance(fac, float):
        n.inputs[0].default_value = fac
    else:
        nt.links.new(fac, n.inputs[0])
    nt.links.new(a, n.inputs[6])
    if b is not None:
        nt.links.new(b, n.inputs[7])
    else:
        n.inputs[7].default_value = b_value
    return n.outputs[2]


def _math(nt, op, a, b=None, clamp=False):
    n = nt.nodes.new("ShaderNodeMath")
    n.operation, n.use_clamp = op, clamp
    for k, v in enumerate((a, b)):
        if v is None:
            continue
        if isinstance(v, (int, float)):
            n.inputs[k].default_value = v
        else:
            nt.links.new(v, n.inputs[k])
    return n.outputs[0]


def _band(nt, value, lo, hi, soft):
    """1 inside [lo, hi] with soft edges, 0 outside."""
    a = _math(nt, "DIVIDE", _math(nt, "SUBTRACT", value, lo - soft), soft, clamp=True)
    b = _math(nt, "DIVIDE", _math(nt, "SUBTRACT", hi + soft, value), soft, clamp=True)
    return _math(nt, "MULTIPLY", a, b)


def metal(name, colour, size, rough=0.36, rough_var=0.12, plates=0.35, metallic=0.75, markings=None,
          tone=0.12, tone_dark=0.9, plate_scale=1.0, tone_mode="panels"):
    """Team-coloured PBR metal: noise-varied roughness, faint plate lines scaled to the ship
    (`size` = 1 at the cruiser's length), optional markings (dorsal strip and cockpit window, in
    object space), and a team_white switch for the overlay pass (render_sprite.py). `tone` sets how
    strongly panels differ in brightness (each panel is randomly 1 or `tone_dark` times the base
    colour); `plate_scale` > 1 makes the panels smaller. With tone_mode "parts", panel lines stay
    faint and each object (part) instead takes one random tone between `tone_dark` and 1, so
    tones follow the ship's parts as in the original rather than running across them."""
    m = bpy.data.materials.new("team_" + name)
    m.use_nodes = True
    nt = m.node_tree
    nodes, links = nt.nodes, nt.links
    bsdf = nodes["Principled BSDF"]
    bsdf.inputs["Metallic"].default_value = metallic
    coords = nodes.new("ShaderNodeTexCoord")

    noise = nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 0.3 / size
    noise.inputs["Detail"].default_value = 5.0
    links.new(coords.outputs["Object"], noise.inputs["Vector"])
    rmap = nodes.new("ShaderNodeMapRange")
    rmap.inputs["To Min"].default_value = rough - rough_var
    rmap.inputs["To Max"].default_value = rough + rough_var
    links.new(noise.outputs["Fac"], rmap.inputs["Value"])
    links.new(rmap.outputs["Result"], bsdf.inputs["Roughness"])

    rgb = nodes.new("ShaderNodeRGB")
    rgb.outputs[0].default_value = (*colour, 1)
    col = rgb.outputs[0]
    if plates > 0:
        bricks = nodes.new("ShaderNodeTexBrick")
        bricks.inputs["Scale"].default_value = 0.1 * plate_scale / size
        bricks.inputs["Mortar Size"].default_value = 0.016
        bricks.inputs["Mortar Smooth"].default_value = 0.4
        bricks.inputs["Brick Width"].default_value = 1.0
        bricks.inputs["Row Height"].default_value = 0.5
        bricks.offset = 0.4
        bricks.inputs["Color1"].default_value = (1, 1, 1, 1)
        bricks.inputs["Color2"].default_value = (tone_dark, tone_dark, tone_dark + 0.01, 1)
        links.new(coords.outputs["Object"], bricks.inputs["Vector"])
        bump = nodes.new("ShaderNodeBump")
        bump.invert = True
        bump.inputs["Strength"].default_value = plates
        bump.inputs["Distance"].default_value = 0.08 * size
        links.new(bricks.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
        if tone_mode == "parts":
            col = _mix_rgb(nt, "MULTIPLY", 0.12, col, bricks.outputs["Color"])
        else:
            col = _mix_rgb(nt, "MULTIPLY", float(tone), col, bricks.outputs["Color"])
    if tone_mode == "parts":
        info = nodes.new("ShaderNodeObjectInfo")
        tmap = nodes.new("ShaderNodeMapRange")
        tmap.inputs["To Min"].default_value = tone_dark
        tmap.inputs["To Max"].default_value = 1.0
        links.new(info.outputs["Random"], tmap.inputs["Value"])
        tint = nodes.new("ShaderNodeCombineColor")
        for k in range(3):
            links.new(tmap.outputs["Result"], tint.inputs[k])
        col = _mix_rgb(nt, "MULTIPLY", 1.0, col, tint.outputs[0])

    shader_out = bsdf.outputs[0]
    if markings:
        sep = nodes.new("ShaderNodeSeparateXYZ")
        links.new(coords.outputs["Object"], sep.inputs[0])
        ax = _math(nt, "ABSOLUTE", sep.outputs["X"])
        top = _math(nt, "GREATER_THAN", sep.outputs["Z"], markings["top_z"])
        if "strip" in markings:
            st = markings["strip"]
            x0, x1 = st.get("x", (-1.0, st.get("hw")))  # a centre band, or a pair at |x| x0..x1
            strip = _math(nt, "MULTIPLY", _band(nt, ax, x0, x1, 0.25 * size),
                          _band(nt, sep.outputs["Y"], *st["y"], 0.8 * size))
            strip = _math(nt, "MULTIPLY", strip, top)
            # a painted band: less metallic, so its base colour sets its tone
            links.new(_math(nt, "SUBTRACT", metallic, _math(nt, "MULTIPLY", strip, 0.55 * metallic)),
                      bsdf.inputs["Metallic"])
            strip_rgb = nodes.new("ShaderNodeRGB")
            strip_rgb.outputs[0].default_value = (*st["colour"], 1)
            col = _mix_rgb(nt, "MIX", strip, col, strip_rgb.outputs[0])
        if "cockpit" in markings:
            ck = markings["cockpit"]
            cockpit = _math(nt, "MULTIPLY", _band(nt, ax, -1.0, ck["hw"], 0.3 * size),
                            _band(nt, sep.outputs["Y"], *ck["y"], 0.4 * size))
            cockpit = _math(nt, "MULTIPLY", cockpit, top)
            glass = nodes.new("ShaderNodeBsdfPrincipled")
            glass.inputs["Base Color"].default_value = (*ck["colour"], 1)
            glass.inputs["Roughness"].default_value = 0.08
            glass.inputs["Coat Weight"].default_value = 1.0
            mixs = nodes.new("ShaderNodeMixShader")
            links.new(cockpit, mixs.inputs[0])
            links.new(bsdf.outputs[0], mixs.inputs[1])
            links.new(glass.outputs[0], mixs.inputs[2])
            shader_out = mixs.outputs[0]

    switch = nodes.new("ShaderNodeValue")
    switch.name = "team_white"
    switch.outputs[0].default_value = 0.0
    col = _mix_rgb(nt, "MIX", switch.outputs[0], col, b_value=(0.8, 0.8, 0.8, 1))
    links.new(col, bsdf.inputs["Base Color"])
    links.new(shader_out, nodes["Material Output"].inputs["Surface"])
    return m


def glow(name, colour, strength):
    """Team-coloured emitter (the originals tint their engine lights too)."""
    m = bpy.data.materials.new("team_" + name)
    m.use_nodes = True
    nt = m.node_tree
    b = nt.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*colour, 1)
    b.inputs["Emission Strength"].default_value = strength
    switch = nt.nodes.new("ShaderNodeValue")
    switch.name = "team_white"
    rgb = nt.nodes.new("ShaderNodeRGB")
    rgb.outputs[0].default_value = (*colour, 1)
    col = _mix_rgb(nt, "MIX", switch.outputs[0], rgb.outputs[0], b_value=(1, 1, 1, 1))
    nt.links.new(col, b.inputs["Emission Color"])
    return m


# ---------------------------------------------------------------------------------------------
# Geometry


def _object(name, bm, mat, bevel=0.0):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(mat)
    for poly in obj.data.polygons:
        poly.use_smooth = True
    if bevel > 0:
        mod = obj.modifiers.new("bevel", "BEVEL")
        mod.width, mod.segments, mod.limit_method = bevel, 3, "ANGLE"
        mod.angle_limit = math.radians(40)
        mod.harden_normals = True
    return obj


def _superellipse_ring(rx, rz_top, rz_bot, zc, e, n):
    t = np.linspace(0, 2 * np.pi, n, endpoint=False)
    c, s = np.cos(t), np.sin(t)
    x = rx * np.sign(c) * np.abs(c) ** (2 / e)
    zs = np.sign(s) * np.abs(s) ** (2 / e)
    z = zc + np.where(zs >= 0, rz_top, rz_bot) * zs
    return x, z


def hull(shape, p, mat, bevel):
    """Loft of the fitted superellipse sections, denser towards the nose, ending in a point."""
    bm = bmesh.new()
    seg = 64
    u = np.linspace(0, 1, 140)
    ys = shape.HULL_REAR + (p["nose_y"] - shape.HULL_REAR) * (1 - (1 - u) ** 1.6)
    rx, rz, zc = shape.hull_profile(p, ys)
    rings = []
    for k, y in enumerate(ys[:-1]):
        x, z = _superellipse_ring(rx[k], rz[k], rz[k] * p["hull_bot"], zc[k], p["hull_exp"], seg)
        rings.append([bm.verts.new((float(x[j]), float(y), float(z[j]))) for j in range(seg)])
    tip = bm.verts.new((0.0, float(ys[-1]), float(zc[-1])))
    for a, b in zip(rings, rings[1:]):
        for j in range(seg):
            bm.faces.new((a[j], a[(j + 1) % seg], b[(j + 1) % seg], b[j]))
    for j in range(seg):
        bm.faces.new((rings[-1][j], rings[-1][(j + 1) % seg], tip))
    bm.faces.new(list(reversed(rings[0])))
    return _object("hull", bm, mat, bevel=bevel)


def prism(name, cx, hw, hh, cz, e, y0, y1, mat, bevel):
    """Superellipse-section prism along Y (the fitted rounded boxes), flat bevelled ends."""
    bm = bmesh.new()
    seg = 64
    x, z = _superellipse_ring(hw, hh, hh, cz, e, seg)
    a = [bm.verts.new((float(cx + x[j]), float(y0), float(z[j]))) for j in range(seg)]
    b = [bm.verts.new((float(cx + x[j]), float(y1), float(z[j]))) for j in range(seg)]
    for j in range(seg):
        bm.faces.new((a[j], b[j], b[(j + 1) % seg], a[(j + 1) % seg]))
    bm.faces.new(a)
    bm.faces.new(list(reversed(b)))
    return _object(name, bm, mat, bevel=bevel)


def build(variant):
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    shape = importlib.import_module(variant + "_shape")
    fit = load_json(variant + "_fit.json")
    p, look = fit["params"], shape.LOOK
    colours = {**DEFAULT_COLOURS, **(load_json(variant + "_colours.json", {}) or {})}
    size = (p["nose_y"] - p["pod_rear"]) / CRUISER_LENGTH
    bevel = look.get("bevel", {})

    rx0, rz0, zc0 = shape.hull_profile(p, np.array([0.0]))
    markings = {"top_z": float(zc0[0]) + 0.35 * float(rz0[0])}
    for key in ("strip", "cockpit"):
        if key in look:
            markings[key] = {**look[key], "colour": colours[key]}
    hull(shape, p, metal("hull", colours["hull"], size, markings=markings), bevel.get("hull", 0.3 * size))

    pod_mat = metal("pods", colours["pods"], size, rough=0.5, rough_var=0.15, plates=0.3)
    top, bot = p["pod_z"] + p["pod_hh"], p["pod_z"] - p["pod_hh"]
    if "groove_z" in look:  # each pod is two stacked nacelles split by a dark groove
        groove, gap = look["groove_z"], 0.45 * size
        nacelles = [((top + groove + gap) / 2, (top - groove - gap) / 2),
                    ((groove - gap + bot) / 2, (groove - gap - bot) / 2)]  # (centre z, half-height)
    else:
        nacelles = [(p["pod_z"], p["pod_hh"])]
    core_mat = metal("pod_core", colours["block"], size, rough=0.5, plates=0.0)
    for side in (-1, 1):
        x = side * p["pod_x"]
        for k, (cz, hh) in enumerate(nacelles):
            prism(f"pod{side:+d}{'ab'[k]}", x, p["pod_hw"], hh, cz, p["pod_exp"],
                  p["pod_rear"], p["pod_front"], pod_mat, bevel=bevel.get("pod", 0.22 * size))
        if len(nacelles) > 1:
            prism(f"pod{side:+d}core", x, p["pod_hw"] - 0.4 * size, gap + 0.3 * size, groove,
                  p["pod_exp"], p["pod_rear"] + 0.3 * size, p["pod_front"] - 0.3 * size, core_mat, 0.0)
    prism("block", 0.0, p["blk_hw"], p["blk_hh"], p["blk_z"], p["blk_exp"], p["blk_rear"], p["blk_front"],
          metal("block", colours["block"], size, rough=0.5, plates=0.0, metallic=0.35),
          bevel=bevel.get("block", 0.2 * size))

    # Engine lights on the back of each pod or nacelle (positions from carve.py).
    red = glow("engine_light", (1.0, 0.04, 0.02), 3.2)
    for side in (-1, 1):
        for cz, hh in nacelles:
            r = min(0.95 * max(size, 0.6), 0.42 * hh, 0.75 * p["pod_hw"])
            bpy.ops.mesh.primitive_cylinder_add(
                location=(side * p["pod_x"], p["pod_rear"] - 0.06 * size, cz),
                radius=r, depth=0.12 * size, rotation=(math.pi / 2, 0, 0), vertices=24)
            bpy.context.active_object.data.materials.append(red)

    return render_settings(fit)
