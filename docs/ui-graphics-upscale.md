# Non-ship graphics: investigation for the AI upscale (2026-10-19)

Read-only investigation, before upscaling anything. Nothing was written to `art/`. The sample
upscales, the overview sheet and the contact sheet are in `out/ui-upscale/` (not tracked).

## 1. What there is

### Pictures (`data/pictures/`, 106 files, all 8-bit palette PNG, none with transparency)

| Group | Count | Typical size | Where it appears |
|---|---|---|---|
| Title and intro | 12 | 400×100 (story stills), 600×480 (starfield backgrounds), 1000×480 (`splash`), 160×36 (`credit`) | `splash`: title screen fade at start (`TitleScreenFade`, `src/ui/flows/master.cpp`). `credit`: "A game by Nathan Lamont" card. `log/*`: the scrolling intro story and the About screen (`intro:`/`about:` in `data/info.pn`, drawn by `build-pix.cpp`), with `log/starfield/a-c` as full-screen backgrounds |
| Main-menu and options frames | 8 | 640×245 top, ~180×211 sides, 640×24 bottom | `gui/main/*`: the main menu room with the ARES sign. `gui/tablet/*`: the frame around Options, Help, level select and the other "tablet" screens (`picture` items in `data/interfaces/*.pn`, drawn by `PictureRect`, `src/ui/widget.cpp`) |
| HUD | 2 | 128×768 left, 32×768 right | `gui/instruments/left/right`: the in-game side panels (`sys.cpp`, drawn in `instruments.cpp`). The radar, bars, menus and text on them are drawn by code |
| Starmap | 1 | 533×364 | Briefing screen map with star names (`briefing.cpp`) |
| Race pictures | 9 | 200×192 | Inline in level briefings (`#+aud/race` in `data/levels/*.pn`, `styled-text.cpp`) |
| Briefing pictures | 15 | 200×100 | Inline in briefings (`brf/*`, `zzz/brf/*`) |
| Object portraits | 59 | 200×100 | Ship and object "portrait" pictures shown in briefing text when an object is described (`BaseObject::portrait`, `styled-text.cpp`) |

### Non-ship sprites (`data/sprites/`, sheet + frame table, same loader as the ships)

| Group | Count | Typical size | Where |
|---|---|---|---|
| Planets | 36 | 64×64, 1 frame | Planets in levels (`loc/planets/*`) |
| Asteroids | 4 | 24 frames, sheets up to 354×240 | Asteroid fields |
| Stations and level objects | ~20 | 1 to 24 frames (`loc/*`, `*/etc/*` stations, labs, devices, `uns/ares`) | Level objects |
| Weapons | 16 | small, 5 to 6 frames (`dev/*`) | Missiles, torpedoes, pulses |
| Effects | 11 | explosions, debris, jump flash (`sfx/*`) | Explosions and hits |
| Cursor | 1 | 12×10 | Mouse cursor (`gui/cursor`) |
| Unused or old | ~23 | `tmp/*`, `zzz/*`, `zrb/*` | Not placed by any campaign level |

### Fonts (`data/fonts/`)

5 bitmap fonts (`button`, `button-small`, `computer`, `tactical`, `title`), 128 to 256 px wide
glyph sheets, each with a hand-made `@2x` already in `data/`.

### Drawn by code (no image file)

Starfield and sparks, sector grid lines, radar/minimap, shield/energy/battery bars, labels and hint
lines, rays and bolts, the tiny map icons (`draw_tiny_*`), and all interface buttons, checkboxes,
tabs and box frames (the coloured bars in Options and the in-game menus; `InterfaceStyle`, text in
the bitmap fonts).

## 2. How they load and scale today

- **Pictures** go through `load_hidpi_texture` (`src/data/resource.cpp` ~292), which searches the
  same order as everything else (plugin, scenario, `art/`, `data/`), so a file in `art/pictures/`
  is picked up, and Graphics: Original hides it. It already looks for `name@{scale}x.png`: it
  starts at the UI scale and halves (4 → @4x, @2x, 1x; 2 → @2x, 1x). The texture is drawn at
  1× logical size, so an `@2x`/`@4x` file must be exactly 2×/4× the 1× size or the layout breaks
  (nothing checks this today). Drawing is nearest-neighbour: at UI N a 1× picture is pixel-blown up
  N×; an `@N×` picture is drawn 1:1.
  - **Gap 1:** at UI 3× it tries `@3x` and then drops straight to 1× (3 >> 1 = 1), so `@2x`/`@4x`
    pictures never show at 3×. Fonts have the same gap. The sprite loader already does this right
    (3 → @4x, @2x, 1x); pictures need the same order.
  - **Gap 2:** `splash`, `starmap`, `credit`, the HUD panels and the fonts are loaded once at
    start-up (`sys_init`, `PluginInit`). Changing UI scale or Graphics in Options does not reload
    them until the game restarts. Interface frames and briefing pictures load when their screen is
    built, so they follow the current settings.
- **Non-ship sprites** use the ship sheet loader (`NatePixTable`): 1×/`@2x`/`@4x` sheets in `art/`
  with the same frame table work now, chosen per draw scale and reloaded every level. The cursor
  is loaded at start-up.
- **Fonts** use `load_hidpi_texture` too (`Resource::font_image`), with the same two gaps.
- **Code-drawn** parts are drawn at logical resolution and blown up N× with nearest-neighbour.
  An image can't improve them; they need drawing changes.

So pictures and sprites need no new kind of support, only the UI 3× fix and (for start-up
pictures) a reload or a restart note. A plain same-size replacement would gain nothing.

## 3. Rating for AI upscaling

| Group | Rating | Why |
|---|---|---|
| Title and intro (`splash`, `log/*` stills and starfields) | **good** | Pre-rendered 3D scenes and space backgrounds, palette dither that the AI removes cleanly |
| Main-menu room (`gui/main/*`) | **good** | Rendered room and ARES sign. Four pieces meet at seams; check the joins |
| Race pictures, briefing pictures, object portraits | **good** | Rendered models on grid backgrounds. The alien-glyph "text" is decorative and survived in the sample |
| Planets, asteroids, stations | **good** | Rendered, dithered sprites; same sheet pipeline as the ships |
| Starmap | **risky** | Mostly real text (star names) and 1-px dotted lines; the sample was readable but bolder |
| Options/tablet frames, HUD panels | **risky** | Rendered metal, but thin, pixel-aligned borders that must meet code-drawn bars and boxes exactly; part of the UI redesign anyway |
| `credit` card | **risky** | Text only |
| Weapons, explosions, effects | **risky** | Small pixel effects, semi-transparent smoke; earlier marked for a separate effects pass |
| Cursor | pointless | 12×10 pixel art |
| Fonts | pointless | Text, hand-made `@2x` already exists; belongs to the UI redesign |
| Code-drawn parts | pointless | No image to upscale |

**Recommendation.** Enhance now: title and intro pictures, the main-menu room, race and briefing
pictures, object portraits, planets, asteroids and stations (about 100 pictures and ~60 sprite
sheets). Before installing, fix gap 1 (UI 3×) and either reload the start-up pictures on a
UI-scale or Graphics change or tell the player a restart is needed (gap 2). Leave the starmap,
Options/tablet frames, HUD, fonts, cursor and code-drawn parts for the UI redesign, and weapons and
effects for the effects pass.

## 4. Sample run

Real-ESRGAN `realesr-animevideov3` (native ×2 and ×4, same tool and model as the ships), on
`splash`, `gui/main/top`, `log/ares-departs`, `log/starfield/a`, `aud/race`, `brf/jump-gate`,
`ish/cruiser` and `starmap`. Borders padded by edge replication so they don't darken. Contact
sheet: `out/ui-upscale/sample-contact.png` (original pixel-doubled | AI @2x | AI @4x shown at 2×).

- **Looks better:** `splash`, the ARES sign in `gui/main/top`, `log/ares-departs`, the race and
  portrait pictures. Dither is gone, edges are clean, lights and coloured details are kept.
- **Artefacts noted:**
  - `log/starfield/a`: the faint nebula smooths out and a few of the faintest stars go (612 of 617
    star peaks kept).
  - `aud/race`: the 1-px yellow grid gets slightly softer (mean difference from the original after
    reducing back to 1×: 4.4 levels of 255).
  - `starmap`: labels stay readable but get bolder; dotted lines kept.
  - Glyph text in `brf/jump-gate` and `ish/cruiser` survives, slightly smoothed.
  - No streaks, fringes or invented detail in the sample.
- **Time:** 8 pictures in 3.7 s at ×2 and 9.6 s at ×4 (RTX 5090), so all 106 take about 3 minutes.

## 5. Cost

- **Disk** (PNG): the sample grows from 0.3 MB (1×, palette) to 4.0 MB @2x and 15.5 MB @4x.
  Extrapolated by area to all 106 pictures: about 14 MB @2x and 52 MB @4x (66 MB for both).
  Planets and other small sprites add a few MB.
- **Memory** (RGBA textures):
  - Pictures loaded at start-up (`splash`, `starmap`, `credit`, HUD panels): 3 MB at 1×, 12 MB
    @2x, 49 MB @4x.
  - Main-menu and tablet frames: 1.2 / 4.7 / 19 MB.
  - Briefing pictures and portraits load per screen; every picture at once would be 16 / 63 /
    252 MB, but the game never holds them all.
  - For comparison, the ship art added about 58 MB of private memory at UI 4× in `ch15`.

Stopped here for review; nothing is installed or committed.

## 6. Loader fixes and the reviewed set (2026-10-19, not committed)

### Loader and drawing fixes

- **UI 3× lookup** (`find_hidpi_image`, `src/data/resource.cpp`): pictures use the sprite-sheet
  order (3× → @4x, @2x, 1x). A `@2x`/`@4x` that isn't exactly 2×/4× the 1× size is skipped with a
  warning. Fonts are pixel art and prefer a scale that divides the screen's (UI 3× → 1× drawn three
  times, so strokes stay even).
- **Start-up textures reload** (`refresh_scaled_textures`, `src/game/sys.cpp`, called every frame
  from the GLFW loop): fonts, HUD panels, cursor, splash and starmap reload when the UI scale or
  the Graphics setting changes; interface pictures (`PictureRect`) reload on their next draw.
- **Layout size** (`OpenGlTextureImpl::size`): a texture now reports its 1× size. Before, a
  `@2x`/`@4x` picture was laid out at its pixel size, 2–4× too big. This never showed because
  `data/` has no hi-res pictures; the first in-game check found it.
- **Filtering** (`TextureFilter`, `include/video/driver.hpp`): 1× textures and fonts stay
  nearest-neighbour (Original looks exactly as before). Hi-res pictures are smoothed when drawn at
  another scale. Hi-res sprite sheets are smoothed when shrunk, with colour bled into clear texels
  so edges don't darken (blending isn't premultiplied).
- **Tests:** `art-test` covers the UI 3× order, a wrong-size `@4x`, fonts, and Graphics: Original
  loading exactly `data/pictures/credit.png` at every scale even with `@2x`/`@4x` in the art folder.
  All 24 tests pass after a clean rebuild.

### Review of the upscaled set

Contact sheets and close-ups: `out/ui-upscale/sheets/`.

- **Clean (13 pictures, 36 sprites):** `log/ares-departs`, `log/ish-encounter`; race pictures
  `aud`, `baz`, `ele`, `gai`, `ish`, `obi`; `brf/aud-device`, `brf/bazidanese`, `brf/habitat`,
  `brf/jump-gate`, `brf/thasero`; asteroids (4), stations and level objects (17), planets
  `bluegas`, `calmpurple`, `clay`, `cloudtones`, `graysplotch`, `jungle`, `lava`, `marshmellow`,
  `marslike`, `moldymud`, `nastyswirl`, `purpleblue`, `purplegas`, `uglypaint`, `yerrear`.
- **Better overall, local artefact (for review):** `splash` (faintest stars lost), the main-menu
  room `gui/main/*` (the small red planet on the wall screen and the orange device get a maze
  texture), `log/starfield/a` (cross-shaped stars become round dots; `b` and `c` are identical
  copies).
- **Excluded:**
  - starmap;
  - small or real text that smears: `log/about-header`, `log/ish-comms`, `brf/uns-ares`,
    `brf/asteroid`, `brf/los-ran`, `brf/relay`, `brf/shipyard`, `brf/thrntz`, `brf/jommo`
    (also a colour shift), `zzz/brf/prctdrne`, `zzz/brf/trgtdrne`, both placeholders;
  - invented maze/worm texture from heavy dither: `log/ares-returns`, `log/ares-christened`, race
    pictures `uns`, `can`, `sal`, planets `aqua-minerale`, `bahamas`, `blueyellow-marble`, `earth`,
    `fall`, `purtygas`, `redwhite-excitement`, `sandyred`, `saturny`, `splatterpaint-pink`,
    `yellow`, `yellow-cookie`; invented crack lines: `bruised`, `marble-streakedpurple`,
    `sta-puft`;
  - all 59 object portraits (dense alien-glyph panels; dithered hulls go wormy).

### Cost

- Clean set: about 10 MB of pictures and 10 MB of sprite sheets on disk (`@2x` + `@4x`); at most
  25 + 23 MB of textures at UI 4× if every one were loaded at once; decoding all of them takes about
  0.1 s (pictures) and 1 s (sprites), and only one size is decoded per scale.
- The review group adds about 27 MB on disk. The `splash` alone is a 31 MB texture at UI 4× and
  stays loaded for the whole session.

### In-game check (UI 1×–4×, 5120-wide window, temporary art folder)

Exclusive fullscreen can't be screen-captured, so the check used a window spanning the 5120-pixel
display, with a 5120×1920 client so that UI 3× and 4× are reachable. **The game caps the UI scale at
height ÷ 480, so on this 1440-pixel-tall display fullscreen tops out at 3×; 4× needs a taller
display.**

- After the layout fix the splash is exactly 1000×480 logical at every scale and centred.
- At UI 3× the `@4x` pictures and sprites are drawn evenly (smooth shrink, no doubled or dropped
  rows).
- The menu room shows no seams between its four pieces; the code-drawn buttons stay crisp.
- Planets and stations draw cleanly at 1× and 3×.
- Not checked in game: the prologue race picture (the capture came before the scroll reached it)
  and memory use.

## 7. Installed (2026-10-19)

The clean set is in `art/` (Git LFS): 13 pictures as `art/pictures/<name>@2x.png` and `@4x.png`, and
36 sprites as `art/sprites/<sprite>/{image,overlay}@{2,4}x.png` (AI image, tolerant-Scale2x overlay).
Checks: every file exactly 2x/4x its 1x original; sprite alpha levels a subset of the original's
(binary, except the sun's 14-level glow); `.pn` files and `data/` unchanged. `ASSETS.md` lists
each as adapted. The loader fixes are in commit `49e96765`. The review group (splash, menu room,
starfield) is not installed; `out/ui-upscale/needs-review/` holds its sheets.

### Checks after installing (2026-10-19)

- **Gaitori race picture** in the ch01 prologue at UI 2x: drawn at its 1x layout size (400x384
  screen pixels) from `art/pictures/gai/race@2x.png` (the capture matches the `@2x` file, mean
  difference 9.4, against 15.6 for the pixel-doubled original).
- **Graphics: Original** on the splash and the main menu at UI 2x: pixel-identical to the build
  from before these changes (`699e25e5`, run with `--no-art`), 0 of 7 million pixels differ.
- **Cost at UI 4x** (`ch15`, 5120x1920 window, private memory):
  - main menu: 299 MB Enhanced, 296 MB Original (no hi-res picture is loaded at start-up with
    this set);
  - after loading the level: 441 MB Enhanced, 330 MB Original (+111 MB, all hi-res ship and
    object sheets);
  - Begin to the briefing screen: 0.88 s Enhanced, 0.86 s Original.
  - Start-up time was not measured reliably (the first-frame detector gave 5.5 s against 0.5 s,
    while memory at the menu was the same and nothing hi-res loads at start-up); to re-measure.
- **Splash faint stars** (review group, not installed): blending each star of the original's dark
  sky back into the AI version as a soft dot (never darkening) restores most of them (63% kept at
  `@4x` before, 86% after) and leaves ships, planets and the logo untouched.

## 8. Start-up, level load and memory, measured (2026-10-19)

Measured with the game's own log (`--timing FILE`: milestones, every image decode, private bytes),
warm file cache (a true cold start needs the Windows file cache emptied, which needs admin rights;
the first run of each mode was no slower than the rest). Start-up: 5 runs per mode at UI 2x,
skipping the title sequence the same way (Esc every 0.3 s). Memory: private bytes of the process
3 s after "level load done", read the same way for every run. 5120-wide window; UI 4x via a
5120x1920 client.

- **Start-up is not slower.** First frame 476 ms Original, 476 ms Enhanced (medians; ranges
  466-487 and 472-490); main menu 1352 and 1339 ms (1314-1359, 1312-1358). Loading itself is done
  by the first frame (about 15 ms of image decoding); the rest is window and OpenGL set-up.
  Enhanced decodes two more files before the menu (the intro stills at `@2x`, about 4 ms).
- **Level load and memory**, before and after the fixes below:

  | Level, UI | Original load | Enhanced load (before → after) | Original MB | Enhanced MB (before → after) |
  |---|---|---|---|---|
  | ch04, 1x | 68 ms | 96 → 80 ms | 282 | 282 → 283 |
  | ch15, 1x | 108 ms | 252 → 191 ms | 289 | 297 → 294 |
  | ch04, 2x | 74 ms | 239 → 155 ms | 286 | 300 → 294 |
  | ch15, 2x | 107 ms | 773 → 486 ms | 296 | 362 → 336 |
  | ch04, 4x | 65 ms | 471 → 291 ms | 322 | 369 → 352 |
  | ch15, 4x | 111 ms | 1105 → 663 ms | 331 | 441 → 397 |

- **What the +111 MB was:** about 45 MB were CPU copies of every hi-res sprite frame, kept after
  the texture was uploaded and never used by the game (now freed). The remaining +66 MB at UI 4x in
  ch15 equals the hi-res textures themselves (18 million pixels, 69 MB as RGBA), which the graphics
  driver also keeps in system memory: that part is the real cost of the art.
- **Fixes (hi-res path only; Original decodes, draws and uses memory as before):**
  1. a hi-res sheet or picture reads its 1x size from the PNG header instead of decoding the 1x
     image (`png_size`, `Resource::sprite_size`);
  2. hi-res sprite frames free their CPU pixel copy after uploading the texture;
  3. the edge-colour bleed for smoothed textures uses direct row access.
  Level load at UI 4x in ch15 is still 0.55 s slower than Original: about 0.4 s is decoding 18
  million pixels of PNG, which only smaller or fewer sheets would reduce.
- **Checked on screen:** after the fixes, each Enhanced level run entered the level at 1:1; the
  ships draw from the hi-res sheets at UI 1x, 2x and 4x, nothing blank or late.

## 9. Review groups installed (2026-10-19)

Your call after the review: all three review groups are in `art/pictures/` (`@2x` and `@4x`, exact
multiples, Git LFS):
- `splash`: the AI version with the original's faint stars blended back (§8 sheet
  `out/ui-upscale/needs-review/splash-2-star-fix-before-after.png`);
- the main-menu room `gui/main/top`, `left`, `right`, `bottom`;
- the intro starfields `log/starfield/a`, `b`, `c` (identical files; only `a` is used).

**Known artefacts kept as they are:**
- the small red planet on the room's wall viewscreen (`gui/main/top`) gets a maze-like texture;
- the orange device in `gui/main/left` gets the same texture;
- the intro starfield's small cross-shaped stars become round dots.

The splash costs a 31 MB texture at UI 4x (8 MB at 2x) for the whole session, since it stays loaded.
