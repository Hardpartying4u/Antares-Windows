# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Render a 3D ship model into an Antares rotation sprite: image + overlay sheets at 1x/2x/4x.

Run inside Blender, from the repository root:

    blender -b --factory-startup [model.blend] -P tools/sprite-render/render_sprite.py -- \
        --sprite uns/cruiser [--model tools/sprite-render/uns_cruiser.py --variant B] \
        [--placeholder cruiser] [--out out/sprite-render]

The original sprite's frame table (data/sprites/<sprite>.pn) is the contract. Every output frame
uses the original frame's rectangle and pivot, so collision bounds and weapon mounts don't change;
the .pn file is never rewritten. The model must be built with its origin at the ship's pivot and
its nose pointing along +Y. Frame i shows the ship turned (360 / frames) * i degrees clockwise,
seen from straight above.

Model conventions (when not using --placeholder):
  - all visible MESH objects in the scene are the ship;
  - objects whose material name starts with "team" take the owner's colour in game (they go into
    the overlay); everything else keeps its own colour;
  - a material may contain a Value node named "team_white": it's set to 1 for the overlay pass and
    0 otherwise, so the material can switch its base colour to white while keeping its roughness
    and normal detail (otherwise the overlay pass uses one flat white material).

A model script (--model) is a Python file with build(variant) that creates the ship in the current
scene and returns a dict of render settings: tilt, lights, ambient (see setup_scene), and
scale (pixels per Blender unit at 1x; fitted to frame 0 if absent), and screen_offset (a constant
(x, y) shift in 1x pixels), and camera_distance (a perspective camera this far from the pivot, in
model units; orthographic if absent).

Outputs, under <out>/sprites/<sprite>/:
  image.png, overlay.png                 1x sheets, same size and layout as the originals
  image@2x.png, overlay@2x.png, ...      higher-resolution sheets, exact multiples of the 1x layout
  fit-report.md                          per-frame fit check against the original frames
"""

import argparse
import math
import os
import re
import struct
import sys
import tempfile
import zlib

import bpy
import numpy as np
from mathutils import Vector

# ---------------------------------------------------------------------------------------------
# Arguments


def parse_args():
    argv = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    p = argparse.ArgumentParser(prog="render_sprite.py", description=__doc__.split("\n")[0])
    p.add_argument("--sprite", required=True, help="sprite name, e.g. uns/cruiser")
    p.add_argument("--data", default="data", help="Antares data directory (default: data)")
    p.add_argument("--out", default="out/sprite-render", help="output directory")
    p.add_argument("--scales", default="1,2,4", help="output scales (default: 1,2,4)")
    p.add_argument(
        "--oversample", type=int, default=2,
        help="render pixels per pixel of the largest output scale (default: 2)")
    p.add_argument("--placeholder", choices=["cruiser"], help="build a placeholder model")
    p.add_argument("--model", help="model script with build(variant), e.g. tools/sprite-render/uns_cruiser.py")
    p.add_argument("--variant", default="", help="variant name passed to the model script")
    p.add_argument("--engine", default="BLENDER_EEVEE", help="BLENDER_EEVEE (default) or CYCLES")
    p.add_argument("--samples", type=int, default=64, help="render samples (default: 64)")
    p.add_argument("--device", default="GPU", help="Cycles device: GPU (OptiX/CUDA if present) or CPU")
    p.add_argument(
        "--light", default=None,
        help="key light azimuth,elevation in degrees, relative to the camera: azimuth 0 = from the"
             " screen top, clockwise (315 = upper left); elevation 90 = from the viewer"
             " (default: 315,50, or the model's)")
    p.add_argument(
        "--tilt", type=float, default=None,
        help="camera tilt from vertical in degrees; positive views the ship from the bottom of"
             " the screen. The original sprites were rendered at a three-quarter view (about 44"
             " for uns/cruiser), which shortens the ship's length on screen (default: 0, or the model's)")
    p.add_argument(
        "--margin", type=float, default=0.5,
        help="extra canvas around each frame, as a fraction of its size, used to measure"
             " clipping (default: 0.5)")
    a = p.parse_args(argv)
    a.data, a.out = os.path.abspath(a.data), os.path.abspath(a.out)  # Blender's loader needs these
    a.scales = sorted({int(s) for s in a.scales.split(",")})
    if a.scales[0] != 1:
        p.error("--scales must include 1")
    return a


# ---------------------------------------------------------------------------------------------
# Antares sprite data


FRAME_RE = re.compile(
    r"left:\s*(-?\d+),\s*top:\s*(-?\d+),\s*right:\s*(-?\d+),\s*bottom:\s*(-?\d+),"
    r"\s*cx:\s*(-?\d+),\s*cy:\s*(-?\d+)")


class Frame:
    def __init__(self, left, top, right, bottom, cx, cy):
        self.left, self.top, self.right, self.bottom = left, top, right, bottom
        self.w, self.h = right - left, bottom - top
        self.px, self.py = cx - left, cy - top  # pivot within the frame, y down


def read_frames(data, sprite):
    with open(os.path.join(data, "sprites", sprite + ".pn"), encoding="utf-8") as f:
        frames = [Frame(*map(int, m)) for m in FRAME_RE.findall(f.read())]
    if not frames:
        raise SystemExit(f"no frames found in {sprite}.pn")
    return frames


def load_rgba(path):
    """Loads a PNG through Blender as a float array [h, w, 4], top row first, values as stored."""
    img = bpy.data.images.load(path, check_existing=False)
    img.colorspace_settings.name = "Non-Color"
    w, h = img.size
    px = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(px)
    bpy.data.images.remove(img)
    return px.reshape(h, w, 4)[::-1].copy()


def write_png(path, rgba):
    """Writes an [h, w, 4] uint8 array as an 8-bit RGBA PNG."""
    h, w, _ = rgba.shape
    raw = b"".join(b"\x00" + rgba[y].tobytes() for y in range(h))

    def chunk(kind, body):
        return (struct.pack(">I", len(body)) + kind + body
                + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF))

    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


# ---------------------------------------------------------------------------------------------
# Scene


def material(name, color, metallic=0.0, roughness=0.5, emission=None):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    if emission:
        bsdf.inputs["Emission Color"].default_value = (*emission, 1.0)
        bsdf.inputs["Emission Strength"].default_value = 4.0
    return m


def clear_scene():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)


def add(prim, name, mat, location, scale, rotation=(0, 0, 0), **kw):
    getattr(bpy.ops.mesh, prim)(location=location, rotation=rotation, **kw)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = scale
    obj.data.materials.append(mat)
    bpy.ops.object.shade_smooth()
    return obj


def build_placeholder_cruiser():
    """A stand-in Human cruiser from basic shapes: a capsule hull with three rear engine pods.

    Proportions follow the original frame 0 (20 x 34 px, pivot 7 px behind the nose), at roughly
    one Blender unit per original pixel: nose at y = +7, engine nozzles at y = -27, width +/-10.
    The original's pivot also sits above the hull: rendered at its three-quarter view (--tilt 44)
    the ship appears about 3.3 px below the pivot in every frame, so the hull's centre line is
    placed 4.7 units below the origin.
    """
    clear_scene()
    hull_mat = material("team_hull", (0.62, 0.62, 0.64), metallic=0.4, roughness=0.35)
    pod_mat = material("team_pods", (0.45, 0.45, 0.47), metallic=0.5, roughness=0.4)
    canopy_mat = material("canopy", (0.05, 0.08, 0.12), roughness=0.15)
    glow_mat = material("engine_glow", (0.9, 0.1, 0.05), emission=(1.0, 0.12, 0.05))
    half_pi = math.pi / 2

    add("primitive_uv_sphere_add", "hull", hull_mat, (0, -6, 0), (3.6, 13, 3.2), segments=48,
        ring_count=24)
    add("primitive_uv_sphere_add", "canopy", canopy_mat, (0, 2.5, 2.2), (1.5, 2.6, 1.0),
        segments=32, ring_count=16)
    for x, length in ((-7, 11), (0, 9), (7, 11)):
        y = -27 + length / 2 + 1.2
        add("primitive_cylinder_add", f"pod{x:+d}", pod_mat, (x, y, 0), (2.8, 2.8, length / 2),
            rotation=(half_pi, 0, 0), vertices=32)
        add("primitive_cylinder_add", f"nozzle{x:+d}", glow_mat, (x, -26.4, 0), (1.6, 1.6, 0.6),
            rotation=(half_pi, 0, 0), vertices=24)
    # struts joining the side pods to the hull
    for x in (-4.2, 4.2):
        add("primitive_cube_add", f"strut{x:+.0f}", pod_mat, (x, -17, 0), (2.4, 2.2, 0.8))
    for obj in bpy.context.scene.objects:
        obj.location.z -= 4.7


def ship_objects():
    return [o for o in bpy.context.scene.objects if o.type == "MESH" and not o.hide_render]


def is_team(obj):
    return any(s.material and s.material.name.lower().startswith("team") for s in obj.material_slots)


def use_gpu():
    """Selects OptiX, CUDA or HIP devices for Cycles if present. Returns the backend, or None."""
    prefs = bpy.context.preferences.addons["cycles"].preferences
    for backend in ("OPTIX", "CUDA", "HIP"):
        devices = prefs.get_devices_for_type(backend) or []
        if any(d.type != "CPU" for d in devices):
            prefs.compute_device_type = backend
            for d in devices:
                d.use = d.type != "CPU"
            return backend
    return None


def camera_light(name, cam, azimuth, elevation, energy, color=(1, 1, 1), angle=5.0):
    """A sun whose direction is fixed relative to the camera (see --light for the angles)."""
    az, el = math.radians(azimuth), math.radians(elevation)
    toward_light = cam.rotation_euler.to_matrix() @ Vector(
        (math.sin(az) * math.cos(el), math.cos(az) * math.cos(el), math.sin(el)))
    sun = bpy.data.objects.new(name, bpy.data.lights.new(name, "SUN"))
    sun.data.energy = energy
    sun.data.color = color
    sun.data.angle = math.radians(angle)  # soft shadows in Cycles
    sun.rotation_euler = (-toward_light).to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.collection.objects.link(sun)


def setup_scene(args, objects, cfg):
    scene = bpy.context.scene
    scene.render.engine = args.engine
    backend = None
    if args.engine == "CYCLES":
        scene.cycles.samples = args.samples
        scene.cycles.use_denoising = True
        if args.device == "GPU":
            backend = use_gpu()
        scene.cycles.device = "GPU" if backend else "CPU"
        if backend == "OPTIX":
            scene.cycles.denoiser = "OPTIX"
    else:
        scene.eevee.taa_render_samples = args.samples
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.image_settings.color_depth = "16"
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.look = "None"
    scene.view_settings.exposure = 0
    scene.view_settings.gamma = 1

    world = scene.world or bpy.data.worlds.new("world")
    scene.world = world
    world.use_nodes = True
    ambient = cfg.get("ambient", 0.04)
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (
        ambient, ambient, ambient * 1.15, 1)

    # Ship root: rotating it turns the whole ship about its pivot (the world origin).
    bpy.context.view_layer.update()  # make matrix_world reflect any pending location changes
    root = bpy.data.objects.new("ship_root", None)
    scene.collection.objects.link(root)
    for obj in objects:
        if obj.parent is None:
            world_matrix = obj.matrix_world.copy()
            obj.parent = root
            obj.matrix_world = world_matrix

    cam = bpy.data.objects.new("camera", bpy.data.cameras.new("camera"))
    cam.data.type = "ORTHO"
    cam.data.clip_start, cam.data.clip_end = 0.1, 1000
    cam.rotation_euler = (math.radians(args.tilt), 0, 0)
    scene.collection.objects.link(cam)
    scene.camera = cam

    # Lights fixed relative to the camera, so lighting stays put on screen while the ship turns.
    if args.light:
        az, el = (float(v) for v in args.light.split(","))
        lights = [{"name": "key", "azimuth": az, "elevation": el, "energy": 4.0}]
    else:
        lights = cfg.get("lights", [{"name": "key", "azimuth": 315, "elevation": 50, "energy": 4.0}])
    for light in lights:
        camera_light(light["name"], cam, light["azimuth"], light["elevation"], light["energy"],
                     light.get("color", (1, 1, 1)), light.get("angle", 5.0))

    white = material("overlay_white", (0.8, 0.8, 0.8), roughness=0.5)
    return root, cam, white, backend, lights


def fit_scale(objects, frame, cam):
    """Pixels per Blender unit that fit the model's camera view into the frame around its pivot."""
    deps = bpy.context.evaluated_depsgraph_get()
    to_cam = cam.rotation_euler.to_matrix().inverted()  # world -> camera axes (x right, y up)
    xs, ys = [], []
    for obj in objects:
        ev = obj.evaluated_get(deps)
        for v in ev.data.vertices:
            p = to_cam @ (ev.matrix_world @ v.co)
            xs.append(p.x)
            ys.append(p.y)
    room = [  # (pixels available, model extent) on each side of the pivot
        (frame.px, -min(xs)), (frame.w - frame.px, max(xs)),
        (frame.py, max(ys)), (frame.h - frame.py, -min(ys)),
    ]
    return min(px / ext for px, ext in room if ext > 1e-6)


# ---------------------------------------------------------------------------------------------
# Rendering


cfg_camera_distance = [None]  # set from the model's settings in main()


def render_frame(args, cam, frame, scale, render_px, tmp, offset=(0.0, 0.0)):
    """Renders the current scene for one frame, with margin. Returns [h, w, 4] float, top first."""
    scene = bpy.context.scene
    margin = math.ceil(max(frame.w, frame.h) * args.margin)
    w, h = frame.w + 2 * margin, frame.h + 2 * margin  # canvas in 1x pixels
    scene.render.resolution_x, scene.render.resolution_y = w * render_px, h * render_px
    distance = cfg_camera_distance[0]
    if distance:
        # Perspective: at the pivot's distance the canvas spans the same width as in orthographic
        # mode, so the scale holds at the pivot while nearer parts look bigger, farther smaller.
        cam.data.type = "PERSP"
        cam.data.sensor_fit = "AUTO"
        cam.data.lens = cam.data.sensor_width * distance / (max(w, h) / scale)
    else:
        cam.data.ortho_scale = max(w, h) / scale
    # Place the camera so the pivot (world origin) lands on the frame's pivot pixel. In camera
    # space (x right, y up) the origin must sit this far from the canvas centre:
    # `offset` nudges the whole image by a constant amount in 1x screen pixels (x right, y down),
    # for originals whose pivots are rounded off-centre.
    origin_cam = Vector(((margin + frame.px + offset[0] - w / 2) / scale,
                         (h / 2 - margin - frame.py - offset[1]) / scale, -(distance or 100)))
    cam.location = -(cam.rotation_euler.to_matrix() @ origin_cam)
    scene.render.filepath = tmp
    bpy.ops.render.render(write_still=True)
    return load_rgba(tmp), margin


def downsample(rgba, factor):
    """Box-filters a float RGBA image by an integer factor, with premultiplied alpha."""
    h, w, _ = rgba.shape
    pre = rgba.copy()
    pre[..., :3] *= pre[..., 3:4]
    pre = pre.reshape(h // factor, factor, w // factor, factor, 4).mean(axis=(1, 3))
    alpha = pre[..., 3:4]
    pre[..., :3] = np.where(alpha > 0, pre[..., :3] / np.maximum(alpha, 1e-6), 0)
    return pre


def to_bytes(rgba):
    return np.clip(np.rint(rgba * 255), 0, 255).astype(np.uint8)


def overlay_from(white_pass):
    """Overlay encoding: red = brightness for the team tint, alpha = coverage."""
    out = np.zeros_like(white_pass)
    out[..., 0] = white_pass[..., :3].mean(axis=2)
    out[..., 3] = white_pass[..., 3]
    return out


def load_model(path, variant):
    """Runs a model script's build(variant); returns its settings dict."""
    import importlib.util
    spec = importlib.util.spec_from_file_location("ship_model", os.path.abspath(path))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.build(variant) or {}


def team_white_nodes():
    return [n for m in bpy.data.materials if m.node_tree
            for n in m.node_tree.nodes if n.name == "team_white"]


def main():
    import time
    started = time.time()
    args = parse_args()
    frames = read_frames(args.data, args.sprite)
    original = load_rgba(os.path.join(args.data, "sprites", args.sprite, "image.png"))
    sheet_h, sheet_w = original.shape[:2]

    cfg = {}
    if args.placeholder == "cruiser":
        build_placeholder_cruiser()
    elif args.model:
        cfg = load_model(args.model, args.variant)
    if args.tilt is None:
        args.tilt = cfg.get("tilt", 0.0)
    cfg_camera_distance[0] = cfg.get("camera_distance")
    objects = ship_objects()
    if not objects:
        raise SystemExit("no mesh objects to render (open a model .blend or use --placeholder)")
    team = [o for o in objects if is_team(o)]
    root, cam, white, backend, lights = setup_scene(args, objects, cfg)
    switches = team_white_nodes()
    # Models built in original-pixel units say so; otherwise fit the model into frame 0.
    scale = cfg.get("scale") or fit_scale(objects, frames[0], cam)

    max_scale = args.scales[-1]
    render_px = max_scale * args.oversample  # render pixels per 1x pixel
    sheets = {s: {k: np.zeros((sheet_h * s, sheet_w * s, 4), np.float32)
                  for k in ("image", "overlay")} for s in args.scales}
    report = []
    tmp = os.path.join(tempfile.mkdtemp(prefix="sprite-render-"), "frame.png")
    view_layer = bpy.context.view_layer

    for i, frame in enumerate(frames):
        root.rotation_euler = (0, 0, -2 * math.pi * i / len(frames))  # clockwise from above

        view_layer.material_override = None
        for n in switches:
            n.outputs[0].default_value = 0.0
        for o in objects:
            o.is_holdout = False
        offset = cfg.get("screen_offset", (0.0, 0.0))
        color, margin = render_frame(args, cam, frame, scale, render_px, tmp, offset)

        # Overlay pass: team parts in white (keeping their detail if the materials support it),
        # non-team parts held out so they hide what's behind them but add no coverage.
        if switches:
            for n in switches:
                n.outputs[0].default_value = 1.0
        else:
            view_layer.material_override = white
        for o in objects:
            o.is_holdout = o not in team
        team_pass, _ = render_frame(args, cam, frame, scale, render_px, tmp, offset)

        for s in args.scales:
            c = downsample(color, render_px // s)
            t = overlay_from(downsample(team_pass, render_px // s))
            m = margin * s
            y, x = frame.top * s, frame.left * s
            sheets[s]["image"][y:y + frame.h * s, x:x + frame.w * s] = c[m:m + frame.h * s, m:m + frame.w * s]
            sheets[s]["overlay"][y:y + frame.h * s, x:x + frame.w * s] = t[m:m + frame.h * s, m:m + frame.w * s]
            if s == 1:
                inside = c[m:m + frame.h, m:m + frame.w, 3]
                total = c[..., 3].sum()
                clipped = max(0.0, (total - inside.sum()) / total) if total else 0.0
                alpha = c[..., 3]
                sides = {  # share of coverage spilling past each edge of the frame
                    "top": alpha[:m].sum(), "bottom": alpha[m + frame.h:].sum(),
                    "left": alpha[:, :m].sum(), "right": alpha[:, m + frame.w:].sum()}
                spill = ", ".join(f"{k} {v / total:.1%}" for k, v in sides.items()
                                  if total and v / total >= 0.001) or "-"
                ys, xs = np.nonzero(inside >= 0.5)
                orig = original[frame.top:frame.bottom, frame.left:frame.right, 3]
                a, b = orig >= 0.5, inside >= 0.5
                iou = (a & b).sum() / max(1, (a | b).sum())
                report.append({
                    "i": i, "deg": i * 360 // len(frames), "w": frame.w, "h": frame.h,
                    "orig_px": int((orig >= 0.5).sum()), "new_px": int((inside >= 0.5).sum()),
                    "bbox": (f"{xs.min()}..{xs.max()} x {ys.min()}..{ys.max()}"
                             if len(xs) else "empty"),
                    "clipped": clipped, "iou": iou, "spill": spill,
                })
        print(f"frame {i:2d}/{len(frames)} done", flush=True)

    out_dir = os.path.join(args.out, "sprites", args.sprite)
    for s in args.scales:
        suffix = "" if s == 1 else f"@{s}x"
        for kind in ("image", "overlay"):
            write_png(os.path.join(out_dir, f"{kind}{suffix}.png"), to_bytes(sheets[s][kind]))

    worst = max(r["clipped"] for r in report)
    ious = [r["iou"] for r in report]
    elapsed = time.time() - started
    model = ("placeholder " + args.placeholder if args.placeholder else
             os.path.basename(args.model) + " variant " + args.variant if args.model else
             bpy.data.filepath)
    lights_text = "; ".join(f"{l['name']} {l['azimuth']:g},{l['elevation']:g} x{l['energy']:g}"
                            for l in lights)
    camera_text = (f"perspective at {cfg['camera_distance']:g} units from the pivot"
                   if cfg.get("camera_distance") else "orthographic")
    lines = [
        f"# Fit check: {args.sprite}",
        "",
        f"- Model: {model}",
        f"- Frames: {len(frames)} ({360 / len(frames):g} degrees each); rectangles and pivots"
        " taken unchanged from the original .pn",
        f"- Scale: {scale:.3f} px per Blender unit at 1x ({'set by the model' if cfg.get('scale') else 'fitted to frame 0'}); scales {args.scales};"
        f" rendered at {render_px} px per 1x px with {args.engine} ({args.samples} samples"
        f"{', ' + backend if backend else ''})",
        f"- Camera: {camera_text}, tilted {args.tilt:g} degrees from vertical; lights {lights_text}",
        f"- Team-colour parts: {', '.join(sorted(o.name for o in team)) or 'none'}",
        f"- Worst clipping: {worst:.2%} of the ship's coverage falls outside its frame"
        f" ({'OK' if worst < 0.005 else 'CHECK'}; threshold 0.5%)",
        f"- Silhouette match with the original (intersection over union at 1x): mean"
        f" {np.mean(ious):.2f}, worst {min(ious):.2f}",
        f"- Render time: {elapsed:.0f} s ({len(frames)} frames x 2 passes)",
        "",
        "| Frame | Angle | Rect | Original px | New px | New bbox (x, y in frame) | IoU | Clipped | Spills past |",
        "|---:|---:|---|---:|---:|---|---:|---:|---|",
    ]
    for r in report:
        lines.append(f"| {r['i']} | {r['deg']} | {r['w']}x{r['h']} | {r['orig_px']} | {r['new_px']}"
                     f" | {r['bbox']} | {r['iou']:.2f} | {r['clipped']:.2%} | {r['spill']} |")
    with open(os.path.join(out_dir, "fit-report.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print(f"wrote {out_dir} (worst clipping {worst:.2%}, mean IoU {np.mean(ious):.2f},"
          f" {elapsed:.0f} s)")


main()
