#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Comparison sheet: an original sprite and its remaster, all frames, at 1x, 2x and 4x.

    python tools/sprite-render/remaster_sheet.py --sprite uns/cruiser \
        --render out/sprite-render/remaster --out out/sprite-render/remaster/comparison.png

For every frame: the original and the remaster side by side at each scale, both at the size the
game draws them at that UI scale (the original pixel-doubled, as the game does without a hi-res
sheet; the remaster from its own @2x/@4x sheet), then a silhouette difference view (grey = both,
red = only the original, blue = only the remaster) with the frame's fit numbers (fit_metric.py).
The header has the overall numbers against the targets. Needs numpy, scipy and Pillow.
"""

import argparse
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFont

import fit_metric

BG = (10, 10, 16, 255)
PANEL = (22, 22, 32, 255)
TEXT = (210, 210, 220)
DIM = (130, 130, 150)
COLS = 6
GAP = 6
DIFF_ZOOM = 4


def font(size):
    for name in ("segoeui.ttf", "arial.ttf", "DejaVuSans.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()


def load(path):
    return Image.open(path).convert("RGBA") if os.path.exists(path) else None


def main():
    a = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    a.add_argument("--sprite", required=True)
    a.add_argument("--data", default="data")
    a.add_argument("--render", required=True)
    a.add_argument("--title", default="")
    a.add_argument("--out", required=True)
    args = a.parse_args()

    frames = fit_metric.read_frames(args.data, args.sprite)
    orig = load(os.path.join(args.data, "sprites", args.sprite, "image.png"))
    base = os.path.join(args.render, "sprites", args.sprite)
    new = {1: load(os.path.join(base, "image.png")), 2: load(os.path.join(base, "image@2x.png")),
           4: load(os.path.join(base, "image@4x.png"))}
    rows, summary = fit_metric.evaluate(frames, fit_metric.mask(os.path.join(args.data, "sprites",
                                        args.sprite, "image.png")), fit_metric.mask(os.path.join(base, "image.png")))

    fw = max(r - l for l, t, r, b, _, _ in frames)
    fh = max(b - t for l, t, r, b, _, _ in frames)
    f_small, f_text, f_head = font(11), font(13), font(20)
    label_h = 16
    cell_w = 2 * 4 * fw + GAP * 3
    cell_h = label_h + sum(s * fh + GAP for s in (1, 2, 4)) + DIFF_ZOOM * fh + 2 * label_h + GAP
    head_h = 120
    n_rows = -(-len(frames) // COLS)
    sheet = Image.new("RGBA", (COLS * (cell_w + GAP) + GAP, head_h + n_rows * (cell_h + GAP) + GAP), BG)
    d = ImageDraw.Draw(sheet)

    def mark(key, higher):
        v, t = summary[key], fit_metric.TARGETS[key]
        return f"{v:.2f} ({'meets' if (v >= t if higher else v <= t) else 'misses'} {t:g})"

    d.text((GAP * 2, 10), args.title or f"{args.sprite}: original vs remaster", font=f_head, fill=TEXT)
    d.text((GAP * 2, 42), f"Silhouette fit, all {len(frames)} frames at 1x: IoU mean {mark('iou_mean', True)},"
           f" worst {mark('iou_worst', True)};  outline distance mean {mark('edge_mean', False)} px,"
           f" worst p95 {mark('edge_p95', False)} px", font=f_text, fill=TEXT)
    d.text((GAP * 2, 64), "Each frame: original (left) and remaster (right) at the size the game draws them at"
           " UI scale 1x, 2x and 4x (original pixel-doubled, remaster from its @2x/@4x sheet).",
           font=f_text, fill=DIM)
    d.text((GAP * 2, 84), "Difference (1x silhouettes, shown 4x):", font=f_text, fill=DIM)
    x0 = GAP * 2 + int(d.textlength("Difference (1x silhouettes, shown 4x):", font=f_text)) + 10
    for colour, name in (((120, 120, 120), "both"), ((255, 80, 60), "only original"), ((60, 160, 255), "only remaster")):
        d.rectangle((x0, 87, x0 + 12, 99), fill=colour)
        d.text((x0 + 17, 84), name, font=f_text, fill=DIM)
        x0 += 30 + int(d.textlength(name, font=f_text))

    om = np.asarray(orig)[..., 3] >= 128
    nm = np.asarray(new[1])[..., 3] >= 128
    for i, (l, t, r, b, _, _) in enumerate(frames):
        w, h = r - l, b - t
        cx = GAP + (i % COLS) * (cell_w + GAP)
        cy = head_h + (i // COLS) * (cell_h + GAP)
        d.rectangle((cx, cy, cx + cell_w - 1, cy + cell_h - 1), fill=PANEL)
        d.text((cx + 6, cy + 2), f"frame {i}  ({i * 360 // len(frames)}°)", font=f_small, fill=TEXT)
        y = cy + label_h
        mid = cx + cell_w // 2
        for s in (1, 2, 4):
            o = orig.crop((l, t, r, b)).resize((w * s, h * s), Image.NEAREST)
            n = new[s].crop((l * s, t * s, r * s, b * s)) if new[s] else None
            sheet.alpha_composite(o, (mid - GAP // 2 - w * s - (fw - w) * s // 2, y + (fh - h) * s // 2))
            if n is not None:
                sheet.alpha_composite(n, (mid + GAP // 2 + (fw - w) * s // 2, y + (fh - h) * s // 2))
            d.text((cx + 4, y), f"{s}x", font=f_small, fill=DIM)
            y += s * fh + GAP
        a_, b_ = om[t:b, l:r], nm[t:b, l:r]
        diff = np.zeros((h, w, 4), np.uint8)
        diff[a_ & b_] = (120, 120, 120, 255)
        diff[a_ & ~b_] = (255, 80, 60, 255)
        diff[b_ & ~a_] = (60, 160, 255, 255)
        dimg = Image.fromarray(diff).resize((w * DIFF_ZOOM, h * DIFF_ZOOM), Image.NEAREST)
        sheet.alpha_composite(dimg, (mid - w * DIFF_ZOOM // 2, y + (fh - h) * DIFF_ZOOM // 2))
        y += fh * DIFF_ZOOM + 2
        rr = rows[i]
        d.text((cx + 6, y), f"IoU {rr['iou']:.2f}   edge {rr['edge_mean']:.2f} px   p95 {rr['edge_p95']:.1f} px",
               font=f_small, fill=TEXT)
        d.text((cx + 6, y + label_h - 2), f"+{rr['extra']} / -{rr['missing']} px   shift {rr['shift'][0]:+.1f},"
               f" {rr['shift'][1]:+.1f}", font=f_small, fill=DIM)

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    sheet.convert("RGB").save(args.out)
    print(f"wrote {args.out} ({sheet.width}x{sheet.height})")


if __name__ == "__main__":
    main()
