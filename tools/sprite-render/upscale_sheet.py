#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Make @2x/@4x sprite sheets by upscaling the original pixels (no 3D).

    python tools/sprite-render/upscale_sheet.py --sprite ish/gunship --method scale2x \
        --out out/sprite-render/upscale/scale2x

Methods:
  nearest  each pixel becomes a 2x2 (4x4) block: the baseline, the same as the game's own doubling.
  scale2x  Scale2x / EPX (Mazzoleni): each pixel becomes 2x2, and a corner takes a neighbour's
           colour where two neighbours agree, smoothing diagonal staircases while copying only
           existing pixels. 4x is Scale2x applied twice (the standard Scale4x).
  scale2x-tol  Scale2x where "agree" means within --tolerance in RGB (xBR-style colour distance),
           so dithered art with few exactly equal pixels still gets its edges smoothed.
  esrgan   Real-ESRGAN (ncnn-vulkan build, --esrgan PATH, --model NAME). An AI model invents new
           pixels, so: each frame is upscaled on black (space) with a margin; image and overlay
           go through the same model; both are then cut with the binary alpha of the tolerant
           Scale2x upscale, so the outline stays clean and identical to that method. Models
           without a native 2x are run at 4x and reduced (Lanczos) for @2x.

The scaler decides from image.png and applies exactly the same source-pixel choice to
overlay.png, so team colour stays aligned with the image, and alpha stays as clean as the
original (pixels are copied, never blended). Each frame rectangle is scaled on its own, with
transparent outside, so neighbouring frames never bleed in. Sheets come out exactly 2x and 4x the
1x size; the frame table (.pn) is untouched. Writes <out>/sprites/<sprite>/ with the original 1x
sheets and the @2x/@4x sheets, ready for override.py. Needs numpy and Pillow.
"""

import argparse
import os
import shutil
import subprocess
import tempfile
import time

import numpy as np
from PIL import Image

import fit_metric


def scale2x_index(img, tolerance=0.0):
    """For an H x W x C image, return (2H x 2W) arrays of source rows and columns (Scale2x).
    Colours closer than `tolerance` (largest RGB channel difference) count as equal."""
    h, w = img.shape[:2]
    pad = np.zeros((h + 2, w + 2) + img.shape[2:], img.dtype)  # transparent border
    pad[1:-1, 1:-1] = img
    flat = pad.reshape(h + 2, w + 2, -1)

    def same(a, b):
        if tolerance <= 0:
            return np.all(a == b, axis=-1)
        d = np.abs(a.astype(int) - b.astype(int))
        alpha_same = (a[..., 3] >= 128) == (b[..., 3] >= 128)
        return alpha_same & (d[..., :3].max(-1) <= tolerance)

    P = flat[1:-1, 1:-1]
    A, B, C, D = flat[:-2, 1:-1], flat[1:-1, 2:], flat[1:-1, :-2], flat[2:, 1:-1]  # up, right, left, down
    rows, cols = np.indices((h, w))
    out_r = np.repeat(np.repeat(rows, 2, 0), 2, 1)
    out_c = np.repeat(np.repeat(cols, 2, 0), 2, 1)
    cond = ~same(A, D) & ~same(C, B)
    # (corner offset, neighbour pair, which neighbour's source position to take)
    for (dy, dx), (n1, n2), (sy, sx) in (
            ((0, 0), (C, A), (-1, 0)),   # top-left: C==A -> A (up)
            ((0, 1), (A, B), (-1, 0)),   # top-right: A==B -> A (up)
            ((1, 0), (D, C), (1, 0)),    # bottom-left: D==C -> D (down)
            ((1, 1), (B, D), (1, 0))):   # bottom-right: B==D -> D (down)
        m = cond & same(n1, n2)
        r_src = np.clip(rows + sy, -1, h)
        c_src = np.clip(cols + sx, -1, w)
        rr = out_r[dy::2, dx::2]
        cc = out_c[dy::2, dx::2]
        rr[m], cc[m] = r_src[m], c_src[m]
    return out_r, out_c


def gather(img, r, c):
    """Pick source pixels; -1 or out-of-range positions are transparent."""
    h, w = img.shape[:2]
    ok = (r >= 0) & (r < h) & (c >= 0) & (c < w)
    out = np.zeros(r.shape + img.shape[2:], img.dtype)
    out[ok] = img[r[ok], c[ok]]
    return out


def upscale_frame(image, overlay, method, factor, tolerance=0.0):
    if method == "nearest":
        r, c = np.indices((image.shape[0] * factor, image.shape[1] * factor))
        return gather(image, r // factor, c // factor), gather(overlay, r // factor, c // factor)
    if method in ("scale2x", "scale2x-tol"):
        for _ in range(factor.bit_length() - 1):
            key = image.copy()
            key[key[..., 3] < 128] = 0  # all transparent pixels count as the same colour
            r, c = scale2x_index(key, tolerance if method == "scale2x-tol" else 0.0)
            image, overlay = gather(image, r, c), gather(overlay, r, c)
        return image, overlay
    raise ValueError(method)


def esrgan_frames(crops, exe, model, factor, pad=4):
    """Upscale RGB crops (on black) with Real-ESRGAN; returns arrays of factor x the crop size."""
    native = {"realesr-animevideov3": (2, 3, 4)}.get(model, (4,))
    run = factor if factor in native else 4
    with tempfile.TemporaryDirectory() as tmp:
        src, dst = os.path.join(tmp, "in"), os.path.join(tmp, "out")
        os.makedirs(src), os.makedirs(dst)
        for k, c in enumerate(crops):
            p = np.zeros((c.shape[0] + 2 * pad, c.shape[1] + 2 * pad, 3), np.uint8)
            p[pad:-pad, pad:-pad] = c
            Image.fromarray(p).save(os.path.join(src, f"{k:03d}.png"))
        subprocess.run([exe, "-i", src, "-o", dst, "-n", model if model != "realesr-animevideov3" else
                        f"realesr-animevideov3-x{run}", "-s", str(run), "-f", "png"],
                       check=True, cwd=os.path.dirname(exe), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        out = []
        for k, c in enumerate(crops):
            im = Image.open(os.path.join(dst, f"{k:03d}.png")).convert("RGB")
            im = im.crop((pad * run, pad * run, (pad + c.shape[1]) * run, (pad + c.shape[0]) * run))
            if run != factor:
                im = im.resize((c.shape[1] * factor, c.shape[0] * factor), Image.LANCZOS)
            out.append(np.asarray(im))
    return out


def upscale_sheet_esrgan(data, sprite, factor, exe, model, tolerance):
    """Real-ESRGAN colours inside the tolerant-Scale2x outline (see the module notes)."""
    base_i, base_o, shape = upscale_sheet(data, sprite, "scale2x-tol", factor, tolerance)
    img = np.asarray(Image.open(os.path.join(data, "sprites", sprite, "image.png")).convert("RGBA"))
    ovl = np.asarray(Image.open(os.path.join(data, "sprites", sprite, "overlay.png")).convert("RGBA"))
    frames = fit_metric.read_frames(data, sprite)

    def on_black(a):
        return (a[..., :3].astype(float) * (a[..., 3:4] / 255.0)).astype(np.uint8)

    ai_i = esrgan_frames([on_black(img[t:b, l:r]) for l, t, r, b, _, _ in frames], exe, model, factor)
    ai_o = esrgan_frames([on_black(ovl[t:b, l:r]) for l, t, r, b, _, _ in frames], exe, model, factor)
    out_i, out_o = base_i.copy(), base_o.copy()
    for (l, t, r, b, _, _), fi, fo in zip(frames, ai_i, ai_o):
        ys, xs = slice(t * factor, b * factor), slice(l * factor, r * factor)
        out_i[ys, xs, :3], out_o[ys, xs, :3] = fi, fo
        out_i[ys, xs][out_i[ys, xs, 3] == 0] = 0  # clean: transparent stays (0, 0, 0, 0)
        out_o[ys, xs][out_o[ys, xs, 3] == 0] = 0
    return out_i, out_o, shape


def upscale_sheet(data, sprite, method, factor, tolerance=0.0):
    img = np.asarray(Image.open(os.path.join(data, "sprites", sprite, "image.png")).convert("RGBA"))
    ovl = np.asarray(Image.open(os.path.join(data, "sprites", sprite, "overlay.png")).convert("RGBA"))
    # Base: nearest everywhere (covers pixels outside frames), then each frame scaled on its own.
    out_i, out_o = upscale_frame(img, ovl, "nearest", factor)
    out_i, out_o = out_i.copy(), out_o.copy()
    for l, t, r, b, _, _ in fit_metric.read_frames(data, sprite):
        fi, fo = upscale_frame(img[t:b, l:r], ovl[t:b, l:r], method, factor, tolerance)
        out_i[t * factor:b * factor, l * factor:r * factor] = fi
        out_o[t * factor:b * factor, l * factor:r * factor] = fo
    return out_i, out_o, img.shape


def main():
    a = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    a.add_argument("--sprite", required=True)
    a.add_argument("--data", default="data")
    a.add_argument("--method", choices=["nearest", "scale2x", "scale2x-tol", "esrgan"], required=True)
    a.add_argument("--esrgan", help="path to realesrgan-ncnn-vulkan.exe (method esrgan)")
    a.add_argument("--model", default="realesr-animevideov3", help="Real-ESRGAN model name")
    a.add_argument("--tolerance", type=float, default=40.0, help="scale2x-tol: RGB distance counted as equal")
    a.add_argument("--out", required=True)
    args = a.parse_args()
    start = time.time()
    dst = os.path.join(args.out, "sprites", args.sprite)
    os.makedirs(dst, exist_ok=True)
    for name in ("image.png", "overlay.png"):
        shutil.copy(os.path.join(args.data, "sprites", args.sprite, name), os.path.join(dst, name))
    for factor in (2, 4):
        if args.method == "esrgan":
            oi, oo, shape = upscale_sheet_esrgan(args.data, args.sprite, factor, args.esrgan, args.model,
                                                 args.tolerance)
        else:
            oi, oo, shape = upscale_sheet(args.data, args.sprite, args.method, factor, args.tolerance)
        assert oi.shape[:2] == (shape[0] * factor, shape[1] * factor)
        Image.fromarray(oi).save(os.path.join(dst, f"image@{factor}x.png"))
        Image.fromarray(oo).save(os.path.join(dst, f"overlay@{factor}x.png"))
    print(f"{args.sprite} {args.method}: wrote @2x/@4x in {time.time() - start:.2f} s -> {dst}")


if __name__ == "__main__":
    main()
