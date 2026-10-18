#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Inventory of the rotating ship sprites, for planning the remaster.

    python tools/sprite-render/fleet_inventory.py --out out/sprite-render/fleet

For every object with a rotating sprite (template chains resolved): its sprite, frame count,
largest frame, draw scale (capital ships 2x) and how often levels use it: the first chapter and
the number of levels that place it or can build it (build lists resolve through the owner's
race). Writes inventory.csv, inventory.md and a contact sheet per faction (frame at 45 degrees,
enlarged 3x, with the sprite name), plus contact sheets for any batch list given with --batches
(a JSON file {"batch name": ["uns/fighter", ...]}). Reads data/ only. Needs Pillow.
"""

import argparse
import csv
import glob
import json
import os
import re
from collections import defaultdict

from PIL import Image, ImageDraw, ImageFont

FRAME_RE = re.compile(r"left:\s*(-?\d+),\s*top:\s*(-?\d+),\s*right:\s*(-?\d+),\s*bottom:\s*(-?\d+),"
                      r"\s*cx:\s*(-?\d+),\s*cy:\s*(-?\d+)")
FACTIONS = {"uns": "Human", "ish": "Ishiman", "can": "Cantharan", "gai": "Gaitori", "sal": "Salrilian",
            "aud": "Audemedon", "obi": "Obish", "baz": "Bazidan", "ele": "Elejeetian", "gro": "Grolk",
            "zrb": "Zorb (unused?)", "ast": "Asteroid", "loc": "Location"}


def parse_object(path):
    """Top-level template, notes and the rotation block (sprite, scale, frames) of an object."""
    text = open(path, encoding="utf-8").read()
    out = {"rotation": {}}
    m = re.search(r'^template:\s*"([^"]+)"', text, re.M)
    out["template"] = m.group(1) if m else None
    for key in ("notes", "long_name", "short_name"):
        m = re.search(rf'^{key}:\s*"([^"]*)"', text, re.M)
        if m:
            out[key] = m.group(1)
    m = re.search(r"^rotation:\s*\{([^}]*)\}", text, re.M)
    if m:
        body = m.group(1)
    else:
        m = re.search(r"^rotation:\s*\n((?:\t.*\n?)*)", text, re.M)
        body = m.group(1) if m else ""
    for key in ("sprite", "scale", "frames"):
        mm = re.search(rf'\b{key}:\s*("([^"]+)"|\{{[^}}]*\}}|[-\d.]+)', body)
        if mm:
            out["rotation"][key] = mm.group(2) or mm.group(1)
    return out


def resolve(name, objects, seen=()):
    o = objects.get(name)
    if o is None or name in seen:
        return {"rotation": {}}
    base = resolve(o["template"], objects, seen + (name,)) if o["template"] else {"rotation": {}}
    merged = {**base, **{k: v for k, v in o.items() if k != "rotation"}}
    merged["rotation"] = {**base["rotation"], **o["rotation"]}
    return merged


def main():
    a = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    a.add_argument("--data", default="data")
    a.add_argument("--out", default="out/sprite-render/fleet")
    a.add_argument("--batches", help="JSON {batch: [sprite, ...]} to draw a contact sheet per batch")
    args = a.parse_args()
    os.makedirs(args.out, exist_ok=True)

    objects = {}
    for path in glob.glob(os.path.join(args.data, "objects", "**", "*.pn"), recursive=True):
        name = os.path.relpath(path, os.path.join(args.data, "objects"))[:-3].replace("\\", "/")
        objects[name] = parse_object(path)

    sprites = defaultdict(lambda: {"objects": [], "scale": 1.0})
    for name in objects:
        if name.startswith("tpl/"):
            continue
        r = resolve(name, objects)
        spr = r["rotation"].get("sprite")
        if not spr or spr == "sfx/invisible":
            continue
        pn = os.path.join(args.data, "sprites", spr + ".pn")
        if not os.path.exists(pn):
            continue
        frames = [tuple(map(int, m)) for m in FRAME_RE.findall(open(pn, encoding="utf-8").read())]
        if len(frames) < 8:  # rotating ships have 24 (or 36) frames
            continue
        s = sprites[spr]
        s["objects"].append(name)
        s["scale"] = max(s["scale"], float(r["rotation"].get("scale", 1.0)))
        s["frames"] = frames
        s["notes"] = s.get("notes") or r.get("notes") or r.get("long_name", "")

    # Level usage: initials name objects directly; build lists name classes per race.
    races = {os.path.basename(p)[:-3] for p in glob.glob(os.path.join(args.data, "races", "*.pn"))}
    obj_sprite = {o: spr for spr, s in sprites.items() for o in s["objects"]}
    usage = defaultdict(lambda: {"levels": set(), "first": None, "placed": 0})
    for path in sorted(glob.glob(os.path.join(args.data, "levels", "*.pn"))):
        text = open(path, encoding="utf-8").read()
        level = os.path.basename(path)[:-3]
        ch = re.search(r"^chapter:\s*(\d+)", text, re.M)
        chapter = int(ch.group(1)) if ch else None
        kind = re.search(r'^type:\s*"(\w+)"', text, re.M).group(1)
        player_races = re.findall(r'^\t\trace:\s*"(\w+)"', text, re.M)
        hits = defaultdict(int)
        for m in re.finditer(r'base:\s*"([^"]+)"', text):
            if m.group(1) in obj_sprite:
                hits[obj_sprite[m.group(1)]] += 1
        for blk in re.finditer(r"\t\tbuild:\n((?:\t\t\t\*.*\n)+)", text):
            for cls in re.findall(r'"([^"]+)"', blk.group(1)):
                for race in player_races:
                    o = f"{race}/{cls}"
                    if o in obj_sprite:
                        hits[obj_sprite[o]] += 0  # buildable: counts as present, not placed
                        usage[obj_sprite[o]]["levels"].add(level)
        for spr, n in hits.items():
            usage[spr]["levels"].add(level)
            usage[spr]["placed"] += n
        for spr in list(usage):
            if level in usage[spr]["levels"] and kind == "solo" and chapter is not None:
                f = usage[spr]["first"]
                usage[spr]["first"] = chapter if f is None else min(f, chapter)

    rows = []
    for spr, s in sorted(sprites.items()):
        fw = max(r - l for l, t, r, b, _, _ in s["frames"])
        fh = max(b - t for l, t, r, b, _, _ in s["frames"])
        u = usage.get(spr, {"levels": set(), "first": None, "placed": 0})
        rows.append({"sprite": spr, "faction": FACTIONS.get(spr.split("/")[0], spr.split("/")[0]),
                     "class": spr.split("/", 1)[1], "notes": s["notes"], "frames": len(s["frames"]),
                     "max_w": fw, "max_h": fh, "draw_scale": s["scale"], "objects": len(s["objects"]),
                     "levels": len(u["levels"]), "first_chapter": u["first"] if u["first"] is not None else "",
                     "placed": u["placed"]})
    with open(os.path.join(args.out, "inventory.csv"), "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0]))
        w.writeheader()
        w.writerows(rows)
    lines = ["| Sprite | Faction | Notes | Frames | Max frame | Scale | Levels | First chapter | Placed |",
             "|---|---|---|---:|---|---:|---:|---:|---:|"]
    for r in rows:
        lines.append(f"| `{r['sprite']}` | {r['faction']} | {r['notes']} | {r['frames']} | {r['max_w']}×{r['max_h']}"
                     f" | {r['draw_scale']:g}× | {r['levels']} | {r['first_chapter']} | {r['placed']} |")
    open(os.path.join(args.out, "inventory.md"), "w", encoding="utf-8").write("\n".join(lines) + "\n")
    print(f"{len(rows)} rotating sprites used by {sum(r['objects'] for r in rows)} objects")

    font = ImageFont.load_default()
    try:
        font = ImageFont.truetype("segoeui.ttf", 14)
    except OSError:
        pass

    def sheet(title, names, path, Z=3):
        cells = []
        for spr in names:
            fr = sprites[spr]["frames"]
            i = len(fr) // 8  # 45 degrees
            l, t, r, b, _, _ = fr[i]
            im = Image.open(os.path.join(args.data, "sprites", spr, "image.png")).convert("RGBA")
            z = Z * (2 if sprites[spr]["scale"] >= 2 else 1)  # as drawn: capital ships at 2x
            cells.append((spr, im.crop((l, t, r, b)).resize(((r - l) * z, (b - t) * z), Image.NEAREST)))
        cw = max(c.width for _, c in cells) + 16
        ch = max(c.height for _, c in cells) + 30
        cols = min(6, len(cells))
        rws = -(-len(cells) // cols)
        out = Image.new("RGBA", (cols * cw, rws * ch + 30), (16, 16, 24, 255))
        d = ImageDraw.Draw(out)
        d.text((8, 6), title, font=font, fill=(225, 225, 232))
        for k, (spr, c) in enumerate(cells):
            x, y = (k % cols) * cw, 30 + (k // cols) * ch
            out.alpha_composite(c, (x + (cw - c.width) // 2, y + 20 + (ch - 30 - c.height) // 2))
            d.text((x + 6, y + 2), spr, font=font, fill=(170, 170, 185))
        out.convert("RGB").save(path)

    by_faction = defaultdict(list)
    for r in rows:
        by_faction[r["sprite"].split("/")[0]].append(r["sprite"])
    for fac, names in by_faction.items():
        sheet(f"{FACTIONS.get(fac, fac)}: frame at 45 degrees, 3x (capital ships drawn at 2x shown 6x)",
              names, os.path.join(args.out, f"faction-{fac}.png"))
    if args.batches:
        for name, names in json.load(open(args.batches, encoding="utf-8")).items():
            sheet(f"Batch {name}", names, os.path.join(args.out, f"batch-{re.sub(r'[^A-Za-z0-9]+', '-', name)}.png"))


if __name__ == "__main__":
    main()
