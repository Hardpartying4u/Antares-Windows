# Star Bound credits

Code authors are listed in `AUTHORS` (upstream Antares). This file credits the third-party tools
and assets used to make Star Bound's art. Asset licences are tracked per file in `ASSETS.md`.

## Original game data

Ares (Nathan Lamont, Bigger Planet) and the Antares project: sprites, sounds, music and text in
`data/`, licensed CC BY-NC-SA 3.0 (`data/COPYING`). Everything in `art/` is derived from them.

## Tools used to make art in `art/`

- **Real-ESRGAN**, by Xintao Wang: the `realesr-animevideov3` model used to upscale ship
  images for the @2x/@4x sprite sheets. BSD 3-Clause License, Copyright (c) 2021, Xintao Wang.
  https://github.com/xinntao/Real-ESRGAN
- **Real-ESRGAN-ncnn-vulkan**, by Xintao Wang: the portable program that ran the model
  (release `realesrgan-ncnn-vulkan-20220424-windows`). MIT License, Copyright (c) 2021 Xintao
  Wang; it includes realsr-ncnn-vulkan, MIT License, Copyright (c) 2019 nihui.
  https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan
- **Scale2x / EPX** (Andrea Mazzoleni's algorithm), reimplemented in
  `tools/sprite-render/upscale_sheet.py`.
- **Blender** (GPL), used to render the 3D remasters (`tools/sprite-render/`).

The tools are not shipped with Star Bound and are not stored in this repository.
