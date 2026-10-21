#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Builds the Windows application icon from the Ishiman Heavy Cruiser's Enhanced art.

    python scripts/make_icon.py [--preview PATH]

Reads art/sprites/ish/hvc/image@4x.png (the AI-upscaled sprite sheet; see ASSETS.md), takes frame 3
(the ship pointing diagonally up and right; frames are 15 degrees apart), and writes:
  * src/win/icon/hvc-source.png: that frame centred on a transparent 256x256 square;
  * src/win/icon/antares.ico: 16, 24, 32, 48, 64, 128 and 256 px.
Sizes up to 32 px are sharpened, their edges made solid and given a dark 1 px outline, so the ship
still reads on light and dark taskbars. --preview writes the 256, 48 and 16 px icons side by side.
The overlay layer (a team-colour mask the game tints at run time) isn't used.
"""

import argparse

from PIL import Image, ImageEnhance, ImageFilter

SHEET = "art/sprites/ish/hvc/image@4x.png"
FRAME = (141, 8, 177, 42)  # frame 3 in data/sprites/ish/hvc.pn (1x coordinates)
SCALE = 4
SIZES = [16, 24, 32, 48, 64, 128, 256]
SOURCE = "src/win/icon/hvc-source.png"
ICO = "src/win/icon/antares.ico"


def clean_edge(ship):
    # The upscaled art has near-black specks along its outer edge: clear the dark pixels that
    # touch transparency (two passes).
    for _ in range(2):
        alpha = ship.getchannel("A")
        near_clear = alpha.point(lambda a: 255 if a < 128 else 0).filter(ImageFilter.MaxFilter(3))
        px, near, out = ship.load(), near_clear.load(), ship.copy()
        o = out.load()
        for y in range(ship.height):
            for x in range(ship.width):
                r, g, b, a = px[x, y]
                if a and near[x, y] and (r + g + b) < 120:
                    o[x, y] = (r, g, b, 0)
        ship = out
    return ship


def source(margin_div=16):
    sheet = Image.open(SHEET).convert("RGBA")
    ship = clean_edge(sheet.crop(tuple(v * SCALE for v in FRAME)))
    ship = ship.crop(ship.getbbox())
    side = max(ship.size)
    square = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    square.alpha_composite(ship, ((side - ship.width) // 2, (side - ship.height) // 2))
    margin = side // margin_div  # a little room at the edges
    framed = Image.new("RGBA", (side + 2 * margin, side + 2 * margin), (0, 0, 0, 0))
    framed.alpha_composite(square, (margin, margin))
    return framed.resize((256, 256), Image.LANCZOS)


def outline(img):
    # A dark ring one pixel wide around the solid shape.
    alpha = img.getchannel("A")
    ring = alpha.filter(ImageFilter.MaxFilter(3))
    edge = Image.new("RGBA", img.size, (8, 24, 32, 0))
    edge.putalpha(ring)
    edge.alpha_composite(img)
    return edge


def icon(src, tight, size):
    img = (tight if size <= 48 else src).resize((size, size), Image.LANCZOS)
    if size <= 32:
        rgb = img.convert("RGB").filter(ImageFilter.UnsharpMask(radius=1, percent=160, threshold=1))
        rgb = ImageEnhance.Contrast(rgb).enhance(1.25)
        alpha = img.getchannel("A").point(lambda a: 255 if a >= 110 else 0)
        img = rgb.convert("RGBA")
        img.putalpha(alpha)
        img = outline(img)
    return img


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview")
    args = parser.parse_args()

    src = source()
    tight = source(margin_div=64)  # small sizes: the ship as large as it can be
    src.save(SOURCE)
    icons = [icon(src, tight, s) for s in SIZES]
    icons[-1].save(ICO, format="ICO", sizes=[(s, s) for s in SIZES], append_images=icons[:-1])

    if args.preview:
        # 256, 48 and 16 px at their real size, on a dark and a light strip.
        shown = [icons[-1], icons[SIZES.index(48)], icons[0]]
        width = sum(i.width for i in shown) + 40 * (len(shown) + 1)
        sheet = Image.new("RGBA", (width, 2 * 296), (0, 0, 0, 255))
        for row, colour in enumerate([(32, 32, 36, 255), (232, 232, 236, 255)]):
            sheet.paste(Image.new("RGBA", (width, 296), colour), (0, row * 296))
            x = 40
            for i in shown:
                sheet.alpha_composite(i, (x, row * 296 + 20 + (256 - i.height) // 2))
                x += i.width + 40
        # Below: the 48 and 16 px icons enlarged 4x (nearest neighbour), to judge their pixels.
        big = [icons[SIZES.index(48)].resize((192, 192), Image.NEAREST),
               icons[0].resize((64, 64), Image.NEAREST)]
        full = Image.new("RGBA", (width, sheet.height + 232), (0, 0, 0, 255))
        full.paste(sheet, (0, 0))
        full.paste(Image.new("RGBA", (width, 232), (96, 96, 104, 255)), (0, sheet.height))
        x = 40
        for i in big:
            full.alpha_composite(i, (x, sheet.height + 20 + (192 - i.height) // 2))
            x += i.width + 40
        full.save(args.preview)


if __name__ == "__main__":
    main()
