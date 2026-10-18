# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Human (UNS) cruiser for render_sprite.py, following docs/art-direction.md.

    blender -b --factory-startup -P tools/sprite-render/render_sprite.py -- --sprite uns/cruiser \
        --model tools/sprite-render/uns_cruiser.py --variant B --engine CYCLES --samples 64

Variants share one silhouette and differ only in how far they're modernised:
  A  subtle:   PBR metal, small bevels, smooth pods, faint rim light, engine glow
  B  balanced: + panel lines, ribbed pods, plate tone variation, stronger rim, running lights
  C  richest:  + greebles (fins, vents, antenna, blisters), edge wear, trim, nav lights, strongest
               rim and glow

Units are about one original pixel. The ship points along +Y with its pivot at the origin:
nose at y = +15.5, engines end at y = -28.5 (44 long, as in the original's side-on frames),
20 wide across the engine pods. The pivot sits above the hull's centre line, as in the original
(see docs/current-state.md section 4.3).
"""

import math

import bmesh
import bpy

NOSE_Y, REAR_Y = 15.5, -26.6        # overall length ~42
HULL_REAR_Y = -16.0                 # hull meets the engine block here
HULL_RX, HULL_RZ = 5.5, 4.3         # hull half-width and half-height at its widest
POD_X, POD_W, POD_H = 6.8, 4.8, 5.2  # pod centre offset, width, height (outer edge +/-9.2)
POD_FRONT_Y = -12.6
Z_DROP = -4.6                       # hull centre line below the pivot
CAMERA_DISTANCE = float(__import__("os").environ.get("UNS_CRUISER_CAMERA", "200"))  # perspective, see build()

SETTINGS = {
    "A": dict(bevel=0.25, rough=0.42, rough_var=0.06, panels=0.0, ribs=False, greebles=False,
              wear=False, rim=1.2, glow=2.6, running=False, nav=False),
    "B": dict(bevel=0.35, rough=0.38, rough_var=0.16, panels=0.55, ribs=True, greebles=False,
              wear=False, rim=2.6, glow=3.4, running=True, nav=False),
    "C": dict(bevel=0.4, rough=0.34, rough_var=0.22, panels=0.85, ribs=True, greebles=True,
              wear=True, rim=3.6, glow=4.2, running=True, nav=True),
}


# ---------------------------------------------------------------------------------------------
# Materials


def _node(nodes, kind, location=(0, 0), **inputs):
    n = nodes.new(kind)
    n.location = location
    for k, v in inputs.items():
        n.inputs[k].default_value = v
    return n


def _mix(nodes, links, location, blend, fac, a, b=None, b_value=None):
    """Colour mix (ShaderNodeMix in RGBA mode): returns the output socket."""
    n = nodes.new("ShaderNodeMix")
    n.location = location
    n.data_type = "RGBA"
    n.blend_type = blend
    if isinstance(fac, float):
        n.inputs[0].default_value = fac
    else:
        links.new(fac, n.inputs[0])
    links.new(a, n.inputs[6])
    if b is not None:
        links.new(b, n.inputs[7])
    else:
        n.inputs[7].default_value = b_value
    return n.outputs[2]


def metal(name, color, cfg, tone=0.0, team=True):
    """Principled metal with roughness variation, optional panel lines, wear and a team switch.

    `tone` shifts the base brightness (plate variation between parts). Team materials get a
    Value node named "team_white" that render_sprite.py sets to 1 for the overlay pass, turning
    the base colour white while keeping roughness, bump and wear.
    """
    m = bpy.data.materials.new(("team_" if team else "") + name)
    m.use_nodes = True
    nt = m.node_tree
    nodes, links = nt.nodes, nt.links
    bsdf = nodes["Principled BSDF"]
    bsdf.inputs["Metallic"].default_value = 0.8
    base = tuple(min(1.0, max(0.0, c + tone)) for c in color)

    coords = _node(nodes, "ShaderNodeTexCoord", (-1200, 0))
    # Roughness variation: low-frequency noise around the base roughness.
    noise = _node(nodes, "ShaderNodeTexNoise", (-900, 200), Scale=0.35, Detail=6.0)
    links.new(coords.outputs["Object"], noise.inputs["Vector"])
    rmap = _node(nodes, "ShaderNodeMapRange", (-650, 200))
    rmap.inputs["To Min"].default_value = max(0.05, cfg["rough"] - cfg["rough_var"])
    rmap.inputs["To Max"].default_value = min(1.0, cfg["rough"] + cfg["rough_var"])
    links.new(noise.outputs["Fac"], rmap.inputs["Value"])
    links.new(rmap.outputs["Result"], bsdf.inputs["Roughness"])

    color_out = None
    rgb = _node(nodes, "ShaderNodeRGB", (-650, -100))
    rgb.outputs[0].default_value = (*base, 1)
    color_out = rgb.outputs[0]

    if cfg["panels"] > 0:
        # Panel lines: mortar of a brick pattern in object space becomes a recessed groove.
        # Large, offset plates (about 9 x 4.5 units) so lines read as panels, not banding.
        bricks = _node(nodes, "ShaderNodeTexBrick", (-900, -400), Scale=0.11,
                       **{"Mortar Size": 0.022, "Mortar Smooth": 0.3, "Bias": 0.0,
                          "Brick Width": 0.9, "Row Height": 0.45})
        bricks.offset = 0.37
        links.new(coords.outputs["Object"], bricks.inputs["Vector"])
        bump = _node(nodes, "ShaderNodeBump", (-400, -400), Strength=cfg["panels"], Distance=0.12)
        bump.invert = True
        links.new(bricks.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
        # Plates vary slightly in tone.
        bricks.inputs["Color1"].default_value = (1, 1, 1, 1)
        bricks.inputs["Color2"].default_value = (0.82, 0.82, 0.84, 1)
        color_out = _mix(nodes, links, (-400, -100), "MULTIPLY", 0.18, color_out,
                         bricks.outputs["Color"])

    if cfg["wear"]:
        # Edge wear (brighter, rougher worn metal on convex edges) and cavity darkening, from
        # Cycles' pointiness.
        geo = _node(nodes, "ShaderNodeNewGeometry", (-900, -700))
        ramp = _node(nodes, "ShaderNodeValToRGB", (-650, -700))
        ramp.color_ramp.elements[0].position = 0.44
        ramp.color_ramp.elements[0].color = (0.55, 0.55, 0.56, 1)  # cavity
        ramp.color_ramp.elements[1].position = 0.58
        ramp.color_ramp.elements[1].color = (1.35, 1.35, 1.35, 1)  # worn edge
        links.new(geo.outputs["Pointiness"], ramp.inputs["Fac"])
        color_out = _mix(nodes, links, (-250, -300), "MULTIPLY", 0.6, color_out,
                         ramp.outputs["Color"])

    if team:
        switch = _node(nodes, "ShaderNodeValue", (-250, 100))
        switch.name = "team_white"
        switch.outputs[0].default_value = 0.0
        color_out = _mix(nodes, links, (-100, 0), "MIX", switch.outputs[0], color_out,
                         b_value=(0.8, 0.8, 0.8, 1))
    links.new(color_out, bsdf.inputs["Base Color"])
    return m


def glossy(name, color, rough=0.12):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*color, 1)
    b.inputs["Roughness"].default_value = rough
    b.inputs["Metallic"].default_value = 0.0
    b.inputs["Coat Weight"].default_value = 0.6
    return m


def emissive(name, color, strength):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*color, 1)
    b.inputs["Emission Color"].default_value = (*color, 1)
    b.inputs["Emission Strength"].default_value = strength
    return m


# ---------------------------------------------------------------------------------------------
# Geometry


def _finish(obj, mat, bevel=0.0, smooth=True):
    obj.data.materials.append(mat)
    if bevel > 0:
        mod = obj.modifiers.new("bevel", "BEVEL")
        mod.width = bevel
        mod.segments = 3
        mod.limit_method = "ANGLE"
        mod.harden_normals = True
    if smooth:
        for p in obj.data.polygons:
            p.use_smooth = True
    return obj


def _mesh_object(name, bm):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def hull(mat):
    """Lofted hull: elliptical sections, full width at the rear, tapering to a pointed nose."""
    bm = bmesh.new()
    rings, seg = 40, 32
    prev = None
    first = None
    for i in range(rings + 1):
        t = i / rings
        y = HULL_REAR_Y + t * (NOSE_Y - HULL_REAR_Y)
        u = max(0.0, (t - 0.50) / 0.50)
        f = (1 - u ** 1.9) ** 0.55 if u < 1 else 0.0
        f = max(f, 0.0)
        ring = []
        for j in range(seg):
            a = 2 * math.pi * j / seg
            # slightly flattened belly, fuller back
            rz = HULL_RZ * (1.0 if math.sin(a) > 0 else 0.85)
            ring.append(bm.verts.new((HULL_RX * f * math.cos(a), y, Z_DROP + rz * f * math.sin(a))))
        if prev:
            for j in range(seg):
                bm.faces.new((prev[j], prev[(j + 1) % seg], ring[(j + 1) % seg], ring[j]))
        else:
            first = ring
        prev = ring
    bm.faces.new(list(reversed(first)))  # rear cap (hidden inside the engine block)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-4)
    return _finish(_mesh_object("hull", bm), mat)


def box(name, mat, center, size, bevel):
    bpy.ops.mesh.primitive_cube_add(location=center)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = (size[0] / 2, size[1] / 2, size[2] / 2)
    bpy.ops.object.transform_apply(scale=True)
    return _finish(obj, mat, bevel, smooth=False)


def cylinder_y(name, mat, center, radius, length, bevel=0.0, verts=32):
    bpy.ops.mesh.primitive_cylinder_add(location=center, radius=radius, depth=length,
                                        rotation=(math.pi / 2, 0, 0), vertices=verts)
    obj = bpy.context.active_object
    obj.name = name
    return _finish(obj, mat, bevel)


def build(variant):
    cfg = SETTINGS[variant.upper()]
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)

    grey = (0.56, 0.57, 0.6)
    hull_mat = metal("hull", grey, cfg, tone=0.04)
    pod_mat = metal("pods", grey, cfg, tone=-0.06)
    block_mat = metal("block", grey, cfg, tone=-0.14)
    trim_mat = metal("trim", (0.3, 0.31, 0.34), cfg)
    canopy_mat = glossy("canopy", (0.02, 0.03, 0.05))
    # Deep red, as the original engine points; kept below the strength where it clips to orange.
    glow_mat = emissive("engine_glow", (1.0, 0.04, 0.02), cfg["glow"])
    vent_mat = emissive("vent_glow", (1.0, 0.05, 0.03), cfg["glow"] * 0.55)

    hull(hull_mat)
    # Canopy: a low dark blister on the back of the hull, behind the pivot.
    bpy.ops.mesh.primitive_uv_sphere_add(location=(0, 3.5, Z_DROP + HULL_RZ - 0.6),
                                         segments=32, ring_count=16)
    canopy = bpy.context.active_object
    canopy.name = "canopy"
    canopy.scale = (1.5, 3.0, 0.9)
    _finish(canopy, canopy_mat)

    pod_len = POD_FRONT_Y - REAR_Y
    pod_cy = (POD_FRONT_Y + REAR_Y) / 2
    for side in (-1, 1):
        x = side * POD_X
        box(f"pod{side:+d}", pod_mat, (x, pod_cy, Z_DROP - 0.3), (POD_W, pod_len, POD_H),
            cfg["bevel"])
        # nozzle glow at the rear and a vent on the pod's back, as the original's two red points
        for dz in (-1.3, 1.3):
            cylinder_y(f"nozzle{side:+d}{dz:+.0f}", glow_mat, (x, REAR_Y + 0.15, Z_DROP - 0.3 + dz),
                       1.0, 0.3, verts=20)
        box(f"vent{side:+d}", vent_mat, (x, -22.0, Z_DROP - 0.3 + POD_H / 2 + 0.02),
            (POD_W * 0.55, 1.2, 0.1), 0.0)
        if cfg["ribs"]:
            for k in range(4):
                y = POD_FRONT_Y - 1.6 - k * 2.6
                box(f"rib{side:+d}{k}", trim_mat, (x, y, Z_DROP - 0.3),
                    (POD_W + 0.25, 0.55, POD_H + 0.25), 0.08)
    # Engine block joining the pods behind the hull.
    box("block", block_mat, (0, -18.8, Z_DROP - 0.6), (2 * POD_X - POD_W + 0.6, 7.6, 4.4),
        cfg["bevel"])

    if cfg["running"]:
        lamp = emissive("running_light", (1.0, 0.95, 0.85), cfg["glow"] * 1.2)
        for side in (-1, 1):
            bpy.ops.mesh.primitive_uv_sphere_add(
                location=(side * (POD_X + POD_W / 2 - 0.5), POD_FRONT_Y - 0.6,
                          Z_DROP - 0.3 + POD_H / 2 - 0.4), radius=0.45, segments=12, ring_count=8)
            _finish(bpy.context.active_object, lamp)
    if cfg["nav"]:
        for side, color in ((-1, (1.0, 0.1, 0.08)), (1, (0.1, 1.0, 0.25))):
            bpy.ops.mesh.primitive_uv_sphere_add(
                location=(side * (POD_X + POD_W / 2 + 0.05), -25.0, Z_DROP - 0.3),
                radius=0.4, segments=12, ring_count=8)
            _finish(bpy.context.active_object, emissive(f"nav{side:+d}", color, 4.0))
        cockpit = emissive("cockpit_glow", (0.55, 0.8, 1.0), 3.0)
        box("cockpit_strip", cockpit, (0, 5.4, Z_DROP + HULL_RZ * 0.93), (1.4, 0.35, 0.1), 0.0)
    if cfg["greebles"]:
        # Radiator fins on the engine block, a dorsal antenna, sensor blisters and hull vents.
        for k in range(4):  # on top of the engine block (y -22.6 .. -15)
            box(f"fin{k}", trim_mat, (0, -16.2 - k * 1.6, Z_DROP + 1.9), (3.4, 0.35, 0.9), 0.05)
        cylinder_y("antenna", trim_mat, (0, -6.0, Z_DROP + HULL_RZ + 0.25), 0.18, 7.0, verts=8)
        for side in (-1, 1):
            bpy.ops.mesh.primitive_uv_sphere_add(
                location=(side * 3.2, -9.0, Z_DROP + 2.3), radius=0.7, segments=16, ring_count=8)
            _finish(bpy.context.active_object, trim_mat)
            box(f"hullvent{side:+d}", trim_mat, (side * 2.6, -12.5, Z_DROP + 2.9),
                (1.4, 2.6, 0.4), 0.06)
            box(f"podplate{side:+d}", trim_mat, (side * POD_X, -26.4, Z_DROP - 0.3 + POD_H / 2),
                (POD_W * 0.8, 2.2, 0.18), 0.05)

    # Fine alignment with the original, measured from silhouette centroids over all 24 frames:
    # the ship sits 0.7 units further back on its pivot. (The originals' pivots are rounded to
    # whole pixels per frame, so the width is kept ~1 px inside the tightest frames.)
    for obj in bpy.context.scene.objects:
        obj.location.y -= 0.7

    return {
        "tilt": 44,
        "camera_distance": CAMERA_DISTANCE,
        "screen_offset": (-0.2, 0.0),  # balances the originals' per-frame pivot rounding
        "scale": 0.97,  # original-pixel units, drawn 3% small to leave ~0.5 px slack
        "ambient": {"A": 0.06, "B": 0.05, "C": 0.045}[variant.upper()],
        "lights": [
            # Key from the upper left (as the originals), slightly warm.
            {"name": "key", "azimuth": 315, "elevation": 48, "energy": 3.6,
             "color": (1.0, 0.97, 0.92), "angle": 6.0},
            # Cool rim grazing from the lower right and slightly behind, outlining the silhouette.
            {"name": "rim", "azimuth": 135, "elevation": -12, "energy": cfg["rim"],
             "color": (0.72, 0.84, 1.0), "angle": 3.0},
            # Soft fill from the viewer so shadowed sides keep readable detail.
            {"name": "fill", "azimuth": 200, "elevation": 70, "energy": 0.5,
             "color": (0.85, 0.9, 1.0), "angle": 30.0},
        ],
    }
