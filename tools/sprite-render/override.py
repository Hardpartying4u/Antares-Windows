#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Install or remove rendered 1x sprite sheets in the scenario override folder, for in-game tests.

    python tools/sprite-render/override.py install --sprite uns/cruiser [--scales 1,2,4] [--from DIR]
    python tools/sprite-render/override.py remove
    python tools/sprite-render/override.py status

Antares looks for resources in the factory scenario folder before its own data folder, so files
placed at <Documents>/Antares/Scenarios/<scenario>/sprites/<sprite>/ replace the originals without
touching data/ or the game code. Only the image and overlay sheets are installed (1x by default;
--scales 1,2,4 adds the @2x/@4x sheets); the original .pn frame table keeps being used. Installed files are recorded in starbound-override.json in the
scenario folder, and "remove" deletes exactly those files (if unchanged) and the folders it made.
Existing files that this tool didn't install are never overwritten.
"""

import argparse
import ctypes
import hashlib
import json
import os
import shutil
import sys

# Factory scenario identifier (kFactoryScenarioIdentifier); its downloaded data lives here.
SCENARIO = "4cab7415715aeeacf1486a352267ae82c0efb220"
MANIFEST = "starbound-override.json"


def sheet_files(scales):
    """File names of the image and overlay sheets at the given scales."""
    names = []
    for s in scales:
        suffix = "" if s == 1 else f"@{s}x"
        names += [f"image{suffix}.png", f"overlay{suffix}.png"]
    return names


def documents_dir():
    """The Documents folder the game uses (SHGetKnownFolderPath(FOLDERID_Documents) on Windows)."""
    if sys.platform != "win32":
        return os.path.expanduser("~/Documents")

    class GUID(ctypes.Structure):
        _fields_ = [("d1", ctypes.c_uint32), ("d2", ctypes.c_uint16), ("d3", ctypes.c_uint16),
                    ("d4", ctypes.c_ubyte * 8)]

    documents = GUID(0xFDD39AD0, 0x238F, 0x46AF,
                     (ctypes.c_ubyte * 8)(0xAD, 0xB4, 0x6C, 0x85, 0x48, 0x03, 0x69, 0xC7))
    path = ctypes.c_wchar_p()
    if ctypes.windll.shell32.SHGetKnownFolderPath(ctypes.byref(documents), 0, None,
                                                  ctypes.byref(path)):
        raise SystemExit("couldn't find the Documents folder")
    try:
        return path.value
    finally:
        ctypes.windll.ole32.CoTaskMemFree(path)


def sha256(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def load_manifest(scenario_dir):
    path = os.path.join(scenario_dir, MANIFEST)
    if not os.path.exists(path):
        return {"files": {}, "dirs": []}
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def save_manifest(scenario_dir, manifest):
    path = os.path.join(scenario_dir, MANIFEST)
    if manifest["files"]:
        with open(path, "w", encoding="utf-8") as f:
            json.dump(manifest, f, indent=2)
    elif os.path.exists(path):
        os.remove(path)


def install(scenario_dir, sprite, source_root, scales):
    files = sheet_files(scales)
    manifest = load_manifest(scenario_dir)
    src_dir = os.path.join(source_root, "sprites", *sprite.split("/"))
    dst_dir = os.path.join(scenario_dir, "sprites", *sprite.split("/"))
    for name in files:
        if not os.path.isfile(os.path.join(src_dir, name)):
            raise SystemExit(f"missing {os.path.join(src_dir, name)}; render the sprite first")
    for name in files:
        rel = f"sprites/{sprite}/{name}"
        if os.path.exists(os.path.join(dst_dir, name)) and rel not in manifest["files"]:
            raise SystemExit(f"refusing to overwrite {rel}: it wasn't installed by this tool")

    # Record each folder we create, deepest last, so "remove" can delete only those.
    d, missing = dst_dir, []
    while not os.path.isdir(d):
        missing.append(d)
        d = os.path.dirname(d)
    for d in reversed(missing):
        os.mkdir(d)
        rel = os.path.relpath(d, scenario_dir).replace(os.sep, "/")
        if rel not in manifest["dirs"]:
            manifest["dirs"].append(rel)

    for name in files:
        dst = os.path.join(dst_dir, name)
        shutil.copyfile(os.path.join(src_dir, name), dst)
        manifest["files"][f"sprites/{sprite}/{name}"] = sha256(dst)
        print(f"installed {dst}")
    save_manifest(scenario_dir, manifest)


def remove(scenario_dir):
    manifest = load_manifest(scenario_dir)
    if not manifest["files"]:
        print("nothing installed")
        return
    for rel, digest in list(manifest["files"].items()):
        path = os.path.join(scenario_dir, *rel.split("/"))
        if not os.path.exists(path):
            print(f"already gone: {path}")
        elif sha256(path) != digest:
            print(f"KEPT (changed since install): {path}")
            continue
        else:
            os.remove(path)
            print(f"removed {path}")
        del manifest["files"][rel]
    for rel in sorted(manifest["dirs"], key=len, reverse=True):
        path = os.path.join(scenario_dir, *rel.split("/"))
        if os.path.isdir(path) and not os.listdir(path):
            os.rmdir(path)
            print(f"removed folder {path}")
            manifest["dirs"].remove(rel)
    if not manifest["files"]:
        manifest["dirs"] = []
    save_manifest(scenario_dir, manifest)


def status(scenario_dir):
    files = load_manifest(scenario_dir)["files"]
    print(f"scenario folder: {scenario_dir}")
    print("\n".join(f"  {rel}" for rel in files) if files else "  no override files installed")


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("action", choices=["install", "remove", "status"])
    p.add_argument("--sprite", help="sprite to install, e.g. uns/cruiser")
    p.add_argument("--scales", default="1", help="sheet scales to install (default: 1)")
    p.add_argument("--from", dest="source", default="out/sprite-render",
                   help="render output root (default: out/sprite-render)")
    p.add_argument("--scenario-dir", help="override folder (default: the factory scenario's)")
    a = p.parse_args()

    scenario_dir = a.scenario_dir or os.path.join(documents_dir(), "Antares", "Scenarios", SCENARIO)
    if not os.path.isdir(scenario_dir):
        raise SystemExit(f"{scenario_dir} doesn't exist; run the game once to download its data")
    if a.action == "install":
        if not a.sprite:
            p.error("install needs --sprite")
        install(scenario_dir, a.sprite, a.source, sorted({int(x) for x in a.scales.split(",")}))
    elif a.action == "remove":
        remove(scenario_dir)
    else:
        status(scenario_dir)


if __name__ == "__main__":
    main()
