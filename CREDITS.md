# Star Bound credits

Code authors are listed in `AUTHORS` (upstream Antares). This file credits the third-party tools
and assets used to make Star Bound's art. Asset licences are tracked per file in `ASSETS.md`.

## Original game data

Ares (Nathan Lamont, Bigger Planet) and the Antares project: sprites, sounds, music and text in
`data/`, licensed CC BY-NC-SA 3.0 (`data/COPYING`). Everything in `art/` is derived from them.

## Modified third-party code

- **GLFW 3.3.2**: `src/glfw/win32-joystick.c` is a modified copy of GLFW's
  `src/win32_joystick.c` (from `ext/glfw/glfw-3.3.2`). On Windows, DirectInput controller detection
  was moved to a worker thread (`docs/startup-timing.md`); the file is marked as altered at its
  top. https://www.glfw.org/

  zlib/libpng License. Copyright (c) 2002-2006 Marcus Geelnard; Copyright (c) 2006-2019 Camilla
  Löwy. This software is provided 'as-is', without any express or implied warranty. In no event
  will the authors be held liable for any damages arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose, including commercial
  applications, and to alter it and redistribute it freely, subject to the following
  restrictions:
  1. The origin of this software must not be misrepresented; you must not claim that you wrote
     the original software. If you use this software in a product, an acknowledgment in the
     product documentation would be appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
     being the original software.
  3. This notice may not be removed or altered from any source distribution.

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

## Application icon

`src/win/icon/antares.ico` (and its source `src/win/icon/hvc-source.png`) shows the Ishiman Heavy
Cruiser from the original Ares art (Bigger Planet Software, CC BY-NC-SA 3.0; see
`data/COPYING`), as upscaled for `art/` with Real-ESRGAN (above). `scripts/make_icon.py` builds it
with Pillow.
