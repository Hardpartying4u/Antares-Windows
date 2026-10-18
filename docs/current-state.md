# Antares: how it works today

A snapshot of the game's controls, command model, UI and rendering as they exist in
the code at upstream `d8ce5826` plus local changes on `windows-working-build` (minimize fix,
integer UI scale). Paths are
relative to the repo root; line numbers are approximate anchors, not contracts.

Two kinds of source define behaviour:

- **C++** in `src/` and `include/` — logic, input handling, drawing.
- **Data** in `data/` (the `antares-data` submodule) — layouts (`data/interfaces/*.pn`),
  sprites, fonts, strings, levels. Many "what is on screen" answers live here, not in code.

---

## 1. Controls

### 1.1 How input flows

```
GLFW (src/glfw/video-driver.cpp)
  key/mouse callbacks → glfw_key_to_usb() → KeyDownEvent / MouseDownEvent …
    → top Card on the stack (src/ui/card.cpp)
        in a mission: GamePlay (src/game/main.cpp ~566)  — global keys (pause, Esc, F-keys)
          → InputSource → PlayerShip (src/game/player-ship.cpp ~328) — ship/command keys
            → PlayerEvent queue → PlayerShip::update() each tick applies them
```

- Keys are stored as **USB HID codes** (`Key` enum, `include/config/keys.hpp`). GLFW keys are
  translated in `glfw_key_to_usb()` (`src/glfw/video-driver.cpp` ~60–160). Mac names are used
  for modifiers: **Option = Alt**, **Command = Windows/Super key**.
- The 44 rebindable actions are the `KeyNum` enum (`include/config/keys.hpp` ~214). Defaults
  are in `Preferences::Preferences()` (`src/config/preferences.cpp` ~43–93). Bindings are saved
  to `config.pn` (`src/config/file-prefs-driver.cpp`) and edited in **Options › Key Controls**.
- Display names for actions: `data/strings/controls/long.pn`; key names:
  `data/strings/keyboard/*.pn`.

### 1.2 Default keyboard bindings (in a mission)

"Keypad" means the numeric keypad. Behaviour column summarises the handler code.

**Ship** (tab "Ship" in Key Controls)

| Action | Default key | What it does |
|---|---|---|
| Accelerate | Keypad 8 | Thrust while held (`ACCEL_ON/OFF` → `kUpKey`) |
| Decelerate | Keypad 5 | Brake while held (`kDownKey`) |
| Turn counter-clockwise | Keypad 4 | Rotate while held (`kLeftKey`) |
| Turn clockwise | Keypad 6 | Rotate while held (`kRightKey`) |
| Fire weapon 1 | Option (Left Alt) | Fire pulse weapon while held (`kPulseKey`) |
| Fire weapon 2 | Command (Left Win) | Fire beam weapon while held (`kBeamKey`) |
| Fire/activate special | Space | Special weapon/device while held (`kSpecialKey`) |
| Warp | Tab | Warp while held. **With Target (Shift) held: engage autopilot** toward the target |

**Command** (tab "Command")

| Action | Default key | What it does |
|---|---|---|
| Select friendly | Keypad Clear | Cycle the **control** selection (yellow label) through friendly ships, nearest in the direction your ship faces. With Target held: *target* a friendly instead |
| Select hostile | Keypad = | Always *targets* (sky-blue label) the next hostile in your facing direction |
| Select base | Keypad / | Select your next base as control object. With Target held: target any base (friend or foe) |
| Target | Shift | A modifier: makes select keys, hotkeys, Warp and mouse clicks act on the **target** instead of the control object. Tapped and held ≥ 0.75 s alone, then released: **targets your own ship** |
| Order to go | Control | Orders the **control** object to go to the current **target** (`kGiveCommandKey` → `PlayerShipGiveCommand`) |
| Scale in / out | Keypad + / Keypad − | Step zoom in / out one level |
| Computer previous / next | Up / Down arrow | Move the minicomputer's line cursor |
| Computer accept | Right arrow | Activate the highlighted minicomputer line |
| Computer cancel | Left arrow | Back out to the minicomputer main menu |

**Shortcuts** (tab "Shortcuts")

| Action | Default key | What it does |
|---|---|---|
| Transfer control | F8 | Switch piloting to the control object (must be your own thinking ship) |
| Zoom to 1:1 / 1:2 / 1:4 / 1:16 | F9 / F10 / F11 / F12 | Jump to that zoom; pressing the same key again returns to the previous zoom |
| Zoom to closest hostile | Help | Auto-scale to fit nearest foe |
| Zoom to closest object | Home | Auto-scale to fit nearest object |
| Zoom to all | Page Up | Fit everything |
| Message next page/clear | Backspace | Advance/clear the on-screen message |

**Utility** (tab "Utility")

| Action | Default key | What it does |
|---|---|---|
| Help | F1 | Opens the in-game Help screen (pauses) |
| Lower volume | F2 | Intended to lower volume — **currently a no-op**: `set_volume(volume())` (`src/game/main.cpp` ~597) |
| Raise volume | F3 | Volume +1 |
| Mute music | F4 | Toggle in-game music (only if "Music During Action" is on) |
| Expert net settings | F5 | No effect (network play not implemented) |
| Fast motion | F6 | While held, game runs 12× speed (`src/game/main.cpp` ~440) |

**Hot keys** (tab "Hot Keys") — 1 … 9, 0

- **Hold ≥ 0.85 s** then release: assign the last selected object to that hotkey (`HOTKEY_SET`).
- **Tap**: select it as control object; with Target held, or if it's not yours, target it.
- Assigned objects show the hotkey number in their label (`name_with_hot_key_suffix`).

**Fixed keys (not rebindable)**

| Key | Where | What it does |
|---|---|---|
| Esc | mission | Opens the Play Again / pause menu (not in replays) |
| Caps Lock | mission | Pauses; the pause screen closes on Caps Lock key-up (`src/game/main.cpp` ~240–330, ~568) |
| Return | mission | Opens a one-line text entry (used for cheat codes and naming objects; `PlayerShip::MessageText`). Esc cancels |
| Alt+Enter | anywhere | Toggle fullscreen/windowed (`GLFWVideoDriver::key`, `src/glfw/video-driver.cpp` ~214) |
| 1–0 | briefing | Show the data sheet for the n-th briefing object |

> **PC keyboard caveat.** GLFW has no key that maps to `Key::N_CLEAR` or `Key::HELP`
> (`glfw_key_to_usb`; Num Lock maps to `NONE`), and most PC keypads have no `=`.
> So **Select friendly, Select hostile and Zoom to closest hostile have no reachable default
> key on Windows/Linux** until rebound in Options › Key Controls.

### 1.3 Mouse

| Input | Where | Effect |
|---|---|---|
| Move | mission | Shows a crosshair cursor sprite; it hides after 1 s idle (`src/game/cursor.cpp`, `kTimeout`). While the cursor is "active", keyboard select/target keys are ignored (`PlayerShip::update`) |
| Left click | play area | Select the friendly object under the cursor (±16 px) as **control** (`PlayerShipHandleClick`, `player-ship.cpp` ~1025) |
| Right click, or left click with Target held | play area | **Target** the object under the cursor (friend or foe) |
| Click / double-click | minicomputer | Highlight a line / execute it (`MiniComputerHandleClick`, `…DoubleClick`) |
| Click | menus | Press buttons, checkboxes, tabs (`src/ui/screen.cpp`, `src/ui/widget.cpp`) |
| Any click / key | intros, fades, scroll text | Skip |

Double-click window is 30 ticks (0.5 s, `kDoubleClickInterval`). There is no scroll-wheel handling.
The OS cursor is hidden; the game draws its own (`GLFW_CURSOR_HIDDEN`). All mouse positions are
in logical pixels: the driver converts window coordinates through the UI scale and border
(`GLFWVideoDriver::to_screen`), so clicks stay exact at every scale.

### 1.4 Gamepad (summary)

Handled in `PlayerShip::gamepad_*` (`player-ship.cpp` ~519–692) and `GamePlay::gamepad_button_down`.
Left stick steers (turns toward stick direction); A thrust, B brake, RT fire 1+2, LT special,
LSB warp toggle, X/Y zoom out/in, Back next message, D-pad drives the minicomputer, Start opens
the Play Again menu. **LB** (target bumper) / **RB** (select bumper) turn A/B/X/Y/LSB into
select/target/order/autopilot/transfer commands aimed with the left stick.
Labels: `data/strings/gamepad/*.pn`.

---

## 2. Player ship, selection and fleet orders

### 2.1 Core model

- **Admiral** (`src/game/admiral.cpp`) = one side (player or AI). It owns cash, a **flagship**,
  a **control** object and a **target** object.
- **Flagship** = the ship you pilot (`g.ship`, attribute `kIsPlayerShip`).
- **Control** (yellow label, `g.control_label`) = what orders apply to.
- **Target** (sky-blue label, `g.target_label`) = where orders send things / what you aim at.
- Selection logic: `select_object` / `pick_object` (`player-ship.cpp` ~200–296), using
  `GetManualSelectObject` / `GetSpritePointSelectObject` to find the next object in the
  facing direction or under the cursor.

### 2.2 Piloting

Each tick `PlayerShip::update()` (~793) folds key events into a bitmask (`gTheseKeys`) and
writes it to the flagship's `keysDown`. The ship's physics/AI then act on `keysDown`
(`src/game/non-player-ship.cpp`, `src/game/motion.cpp`). `g.key_mask` lets levels
(e.g. tutorials) disable specific keys.

- Low shields (< 25 % health) trigger a klaxon and a "shields low" message every ~2 s.
- **Autopilot**: Shift+Tab (or gamepad) sets `kAutoPilotKey | kAdoptTargetKey` — the flagship
  adopts the target as its destination and flies itself. Pressing any steering key while on
  autopilot toggles it off (`handle_pilot_keys`). Toggle logic: `TogglePlayerAutoPilot` (~1102).
- If the flagship dies, control passes per `PlayerShipBodyExpire` (~1130).

### 2.3 Orders

| Order | How | Code |
|---|---|---|
| Go to target | Control key (Order to go), or gamepad | `SetObjectDestination(control)` → `OverrideObjectDestination` (`admiral.cpp` ~433): destination = admiral's target |
| Transfer control | F8 or minicomputer | `transfer_control` (`minicomputer.cpp` ~815) → `ChangePlayerShipNumber` |
| Hold position | minicomputer › Special Orders | `hold_position`: destination = its own location |
| Go to my position | minicomputer › Special Orders | `come_to_me`: destination = flagship location |
| Fire weapon 1 / 2 / special | minicomputer › Special Orders | `fire_weapon`: control ship fires with `kManualOverrideFlag` |
| Build ship | minicomputer › Build | `build_ship` → `Admiral::build` at the selected base, if affordable and under the ship cap |

Objects only accept orders if they have `kCanAcceptDestination` and no `kStaticDestination`.

### 2.4 Minicomputer

The green text panel in the lower-left instrument panel (`src/game/minicomputer.cpp`;
strings in `data/strings/minicomputer.pn`). 10 lines × 25 chars at panel offset (12, 320).

- **Main Menu**: Build · Special Orders · Message · Mission Status
- **Build Ships**: the selected base's name, then the ship types it can build; types you can't
  afford are dimmed. The highlighted ship's price is shown on the money bars, and a build-time bar
  appears in the right panel (`draw_build_time_bar`)
- **Special Orders**: Transfer Control · Hold Position · Go To My Position · Fire Weapon 1 ·
  Fire Weapon 2 · Fire Special
- **Messages**: Next Page/Clear · Previous Page · Last Message
- **Mission Status**: level-defined status lines
- The top lines always show the current **CONTROL** and **TARGET** objects.

### 2.5 Timing

60 ticks per second; every 3rd tick is a "major" tick when collisions, AI and actions run
(`include/math/units.hpp` ~30–47). Game loop: `GamePlay::fire_timer` (`src/game/main.cpp` ~420–560).

---

## 3. Screens and menus

### 3.1 Architecture

- UI is a **stack of Cards** (`src/ui/card.cpp`); the top card gets input and is drawn over the
  ones below. Flows push/pop cards as state machines.
- Most menus are **InterfaceScreens** (`src/ui/screen.cpp`) built from a layout file in
  `data/interfaces/`. Each layout lists widgets (`button`, `checkbox`, `tab-box`, `rect`,
  `picture`, `text`) with pixel `bounds`, a `label`, a keyboard `key` and a `gamepad` button.
- Layouts are in a **640 × 480 coordinate space, centered** in the logical screen
  (`InterfaceScreen::offset`, `screen.cpp` ~181), which the UI scale magnifies (§4.1).
  `fullscreen: true` clears the rest of the screen to black; `false` overlays the card underneath.
- Screens can add widgets in code with `InterfaceScreen::add_widget` (used for the UI-scale
  choices, so the separately licensed `data/` layouts stay unmodified).
- Widget drawing (bracketed frames, button styles, hues): `src/drawing/interface.cpp`.

### 3.2 Flow

```
MasterFlow (src/ui/flows/master.cpp)
  publisher picture → "ego" picture → title fade → intro scroll text → MainScreen
MainScreen
  ├─ Start New Game → SoloGame (src/ui/flows/solo-game.cpp)
  │     SelectLevel → prologue scroll text → MainPlay (src/game/main.cpp):
  │        LoadingScreen → BriefingScreen → GamePlay → Debriefing / PlayAgain
  │     → next chapter … → epilogue scroll text
  ├─ Replay Intro / About Ares → ScrollTextScreen
  ├─ Demo → ReplayGame (src/ui/flows/replay-game.cpp) — plays a recorded replay
  ├─ Options → OptionsScreen (sound ⇄ video ⇄ keys)
  └─ Quit → fade out
  (idle on MainScreen → random demo replay or intro, via MainScreen::fire_timer)
```

### 3.3 Each screen

| Screen | Code | Layout | Contents |
|---|---|---|---|
| Publisher / ego / title pictures | `ui/flows/master.cpp`, `PictFade`, `TitleScreenFade` | `data/pictures` | Full-screen fades; any key skips |
| Scroll text (intro, about, prologue/epilogue) | `ui/screens/scroll-text.cpp` | — | Picture-and-text crawl; slow/fast speeds |
| **Main menu** | `ui/screens/main.cpp` | `interfaces/main.pn` | Artwork frame (top/left/right/bottom pictures) around a centered column (x 207–427): **Start New Game** (S), Start Network Game (N, always disabled — throws "not yet implemented"), Replay Intro (R), Demo (D), Options (O), About Ares (A), Quit (Q) |
| **Select level** | `ui/screens/select-level.cpp` | `interfaces/select-level.pn` | "Select Level" tablet frame; chapter name/info box; Previous (←) / Next (→) at y 302; Cancel (Esc) / Begin (Return) at y 422. Only unlocked chapters (`config/ledger.cpp`) |
| **Loading** | `ui/screens/loading.cpp` | `interfaces/loading.pn` | Small "Loading…" box with progress |
| **Briefing** | `ui/screens/briefing.cpp` | `interfaces/briefing.pn` | "Mission Analysis" tablet: star map → blank system map → each briefing point (map with highlighted objects + text). Previous (←), Next (→), Done (Return); Esc cancels. Keys 1–0 open object data sheets |
| Object data | `ui/screens/object-data.cpp` | — | Pop-up spec sheet for a ship/base, generated text typed out "retro" style |
| **In-game** | `game/main.cpp` (`GamePlay`) | code (see 3.4) | Play area + instrument panels |
| Pause | `game/main.cpp` (`PauseScreen`) | — | Blinking "paused" string over the game |
| Help | `ui/screens/help.cpp` | `interfaces/help.pn` | Overlay box (480 × 460) showing the `help` text resource with your current key names substituted in; Done (Esc) |
| **Play again** | `ui/screens/play-again.cpp` | `interfaces/play-again/{normal,lose,tutorial}.pn` | Centered dialog: Resume (Esc), Start Chapter Over (Return), Quit to Main Menu (Q); tutorial variant adds Skip Tutorial (S); lose variant has no Resume |
| **Debriefing** | `ui/screens/debriefing.cpp` | — | Typed-out victory/defeat text plus score table: your time / losses / kills vs par; score = time(50) + losses(30) + kills(20) |
| **Options › Sound** | `ui/screens/options.cpp` ~137 | `interfaces/options/sound.pn` | Checkboxes: Music During Action, Music During Interlude, Speak Network Messages; Volume Up/Down + volume bar; Key Controls (K), Video (V), Cancel (Esc), Done (Return) |
| **Options › Video** | `options.cpp` ~231 | `interfaces/options/video.pn` + code | Fullscreen checkbox (applies next launch, or Alt+Enter); a column of **UI Scale** choices — Auto, 1x, 2x, 3x, 4x — that behave as radio buttons and apply immediately (added in code, `kUiScaleChoices`); Key Controls, Sound, Cancel (reverts), Done |
| **Options › Key Controls** | `options.cpp` ~294 | `interfaces/options/keys.pn` | Tab box with 5 tabs (Ship, Command, Shortcuts, Utility, Hot Keys); each action is a button showing its key — click it, press a key to bind it, and selection advances to the next action (Esc/Return/Caps Lock cancel). Conflicting bindings are flagged in a "conflict" text area and block Done/Options until fixed. Cancel, Options, Done |

### 3.4 In-game screen layout

All in logical pixels, from `include/game/globals.hpp`, `src/game/main.cpp` ~78–88 and
`src/game/instruments.cpp`:

```
┌──────────┬─────────────────────────────────────────────┬───┐
│  LEFT    │                                             │ R │
│  PANEL   │              PLAY AREA (viewport)           │ I │
│  128 px  │   starfield, sprites, beams, labels,        │ G │
│ ┌──────┐ │   status messages, direction "site"         │ H │
│ │radar │ │                                             │ T │
│ └──────┘ │                                             │   │
│  money   │                                             │32 │
│  bars    │                                             │px │
│ ┌──────┐ │                                             │   │
│ │mini- │ ├─────────────────────────────────────────────┤   │
│ │comp. │ │ long-message area (bottom_border, varies)   │   │
└─┴──────┴─┴─────────────────────────────────────────────┴───┘
```

- **Play area** = logical screen width − 128 − 32, full height (`play_screen()`, `viewport()`).
  It never stretches: zoom is one scale factor for both axes. A bigger *logical* screen shows
  more of the battlefield. Fixed zooms show more in both directions; auto zooms (closest foe,
  closest object, all) fit by height only, so extra width is extra view. The UI scale (§4.1)
  keeps the logical screen near 640–1080 px tall, but wide windows still see more horizontally.
  The simulation never depends on the view.
- The panel artwork is 768 logical px tall, centered vertically. When the logical screen is
  shorter (for example 540 at 1080p with 2× UI scale), its decorative top and bottom are
  trimmed. The functional parts sit in the middle 480 px and stay visible.
- **Left panel** (texture `sys.left_instrument_texture`, centered in a 768-px band):
  radar (110 × 110 at top, green blips), gross/fine **money** bars (yellow/pale-green; orange =
  needed, sky-blue = spent), **minicomputer** at y 320.
- **Right panel**: three vertical bars — **battery** (salmon, y 103), **energy** (gold, y 231),
  **shields** (sky-blue, y 359) — plus ammo counters and the build-time bar.
- **Messages** (`src/game/messages.cpp`): short status labels (aqua, top), red tactical message
  line, and a long-message box at the bottom that pushes the viewport up (`g.bottom_border`).
- **Zoom levels** (`Zoom`, `include/data/enums.hpp`): 2:1, 1:1, 1:2, 1:4, 1:16, closest-foe,
  closest-object, all. Missions start at closest-foe (`set_up_instruments`).

---

## 4. Graphics

### 4.1 Platform layer

- **Window/context**: GLFW (`src/glfw/video-driver.cpp`). Tries **OpenGL 3.2 core (GLSL 330)**,
  falls back to **OpenGL 2.0 (GLSL 110)** (`hint_opengl32/20`, ~455–470). On MSVC, GL entry points
  are loaded at runtime (`src/video/opengl-dynamic.cpp`).
- **Resolution and UI scale** (`GLFWVideoDriver::update_layout`, `src/glfw/video-driver.cpp`):
  everything is drawn on a **logical screen** = framebuffer ÷ `scale`, where
  `scale` = UI scale × pixel ratio (pixel ratio is 1 on Windows, 2 on a Mac Retina display).
  - UI scale: preference `ui_scale` (`video: "ui scale"` in `config.pn`; 0 = Auto). Auto =
    ⌊window height ÷ 540⌋, minimum 1. Any choice is capped so the logical screen stays at least
    640 × 480. Recomputed every frame, so window resizes and Options changes apply at once.
  - The logical screen is magnified with nearest-neighbour filtering into a viewport centered in
    the framebuffer; leftover pixels (fewer than `scale`) form a black border
    (`viewport_origin()`, used by `OpenGlVideoDriver::MainLoop::draw`).
  - Mouse positions are converted from window coordinates to logical pixels in `to_screen()`.
  - Window size (saved in prefs; fullscreen = monitor mode) is tracked separately from the
    logical screen size.
  - Textures with `@Nx` variants are chosen by `load_hidpi_texture` (`src/data/resource.cpp`) at
    load time, which happens once at startup. Only fonts ship `@2x` art, so a later scale change
    upscales whatever was loaded. At 3× it falls back to 1× art (it tries `@3x`, then halves).
    Sprites use their own, better rule (§4.3) and are loaded when a level loads.
- **Saved window size** is DPI-independent: the window is created with `GLFW_SCALE_TO_MONITOR`,
  which multiplies the requested size by the monitor's content scale on Windows, so the driver
  saves window size ÷ that scale (`saved_size_ratio()`). At 125 % a 1920 × 1080 window is saved
  as 1536 × 864 and reopens at 1920 × 1080. (Before this, the saved size grew 25 % per launch
  after any resize.) Configs saved by older builds hold the physical size, so they open
  25 % larger once.
- **Screenshots**: GDI and Win+PrintScreen capture a black frame in fullscreen (OpenGL surface).
  Use a window the size of the monitor when screenshots are needed.
- **Build note**: the Windows toolchain doesn't pass `/showIncludes`, so ninja doesn't track
  header dependencies. After changing a header, do a clean rebuild (`ninja -t clean`) or
  stale objects can crash at runtime.
- **Main loop**: poll events → draw whole card stack → swap → fire timers
  (`GLFWVideoDriver::loop`, ~473). No explicit vsync setting.

### 4.2 Renderer

`src/video/opengl-driver.cpp` with shaders `src/video/glsl/vertex.vert` and `fragment.frag`
(embedded at build time via `embed()` in `BUILD.gn`).

- One shader program, pixel-space orthographic projection, **nearest-neighbour** filtering,
  standard alpha blending. All colours are converted **Apple RGB → sRGB** in the fragment shader
  (the original art was authored on Macs).
- Six colour modes:

| Mode | Used for |
|---|---|
| `FILL` | Rects, lines, points (UI frames, stars, radar, bars, beams) |
| `DITHER` | Half-alpha fills |
| `DRAW_SPRITE` | Plain sprites, pictures, fonts |
| `TINT_SPRITE` | Coloured text and tinted images |
| `STATIC_SPRITE` | **Hit flash** (shield colour) and **cloaking** — mixes the sprite with a 256 × 256 random noise texture by `static_fraction` |
| `OUTLINE_SPRITE` | Outlined object icons on briefing maps (`briefing.cpp` ~401) |

- Other drivers: `src/video/offscreen-driver.cpp` (headless, used by tests),
  `src/video/text-driver.cpp` (logs draw calls for replay tests).

### 4.3 Sprites

- Data: `data/sprites/<race-or-set>/<name>/{image.png, overlay.png}` plus `<name>.pn` listing
  each **frame** rectangle and its centre (`cx`, `cy`). 382 PNGs in total.
- Frames are usually **rotation steps** (ships, turrets) or animation frames (explosions,
  shots). Facing → frame index is computed from the object's direction.
- **Rotation sprites in detail:**
  - An object's `rotation: {sprite, frames: {begin, end}}` block picks the sheet. The angle step
    is 360° ÷ frame count (`rotation_resolution`, `src/game/space-object.cpp` ~882).
  - 78 distinct rotating sprites are in use, by 106 objects once template chains are resolved
    (`tools/sprite-render/fleet_inventory.py`, 2026-10-18; an earlier count of 82 objects missed
    the template chains). All have 24 frames (15°) except the Bazidan battleship, which has 36
    (10°). Frames range from 12 × 11 px (fusion pulse) to 108 × 78 px (Human carrier); the Human
    cruiser's are 20–44 × 21–34 on a 342 × 188 sheet.
  - Frame 0 shows the nose pointing **up**; each next frame turns the ship **clockwise**.
  - `cx, cy` are absolute sheet coordinates of the **pivot**, i.e. the object's position, which
    weapon mounts are measured from. It sits near the nose, not at the frame's middle.
  - Frames are packed about six per row with irregular spacing; only the `.pn` rectangles matter.
  - The originals are palette PNGs, but the loader accepts any PNG, including 8-bit RGBA.
  - `overlay.png` has the same layout: red = brightness for the team tint, alpha = coverage. On
    the originals it covers the whole hull.
- **The originals use a three-quarter view.** Fitting each frame's pivot against the ship's
  position shows a camera tilted about 44° from vertical for the Human cruiser, viewing from the
  bottom of the screen; the ship's length appears 0.72× on screen. The pivot also sits above the
  hull (about 3.3 px on screen). Other sprites give noisier estimates (8°–52°).
  The camera was also a **perspective** camera: parts farther from it, such as the rear of a
  ship facing down the screen, look slightly smaller. For the Human cruiser a camera about 200
  units (ship lengths ≈ 44) from the pivot fits best. An orthographic camera can't get every
  frame within 1 %.
- **Pivots are rounded per frame.** The originals' `cx, cy` are whole pixels, so the ship sits up
  to about 1 px off-centre in individual frames. A consistent model needs about 0.5 px of slack
  to stay inside every frame.
- **Players see `image.png`.** A side is tinted only if its level sets a `hue`, and almost none do
  (default grey draws the image as painted). The overlay matters only for hued sides. See
  `docs/art-direction.md`.
- **Resource search order:**
  1. plugin folder or zip;
  2. the factory scenario folder (`Documents/Antares/Scenarios/<id>`);
  3. the Star Bound art folder (`art_path()`; added 2026-10-18);
  4. the app's `data/` (`src/data/resource.cpp` ~89).

  A file in the scenario folder therefore replaces the same-named file everywhere else, which the
  sprite tools use for in-game tests. The art folder holds finished replacement art (`art/`).
  - The game uses `art/` next to the data folder when it exists. `--art DIR` points elsewhere;
    `--no-art` shows the originals (`src/win/main.cpp`).
  - Tests and tools never set it, so they always see the original art.
  - `scripts/dist.py` packages `art/` next to `data/`.
  - Each file is looked up on its own, so a sprite takes its sheets from `art/` and its frame table
    from `data/`.
  - Tested in game: by default the Human fighter, cruiser and carrier draw from `art/`; with
    `--no-art` they show the originals.
- **Team colour**: `NatePixTable(name, hue)` (`src/drawing/pix-table.cpp` ~38) tints the
  overlay layer with the owner's hue and composites it onto the image; gray = untinted.
- Drawing: `draw_sprites()` (`src/drawing/sprite-handling.cpp` ~168) draws three layers in order
  **BASES → SHIPS → SHOTS**, scaled by zoom. Below 1:4 zoom (`kBlipThreshhold`), sprites are
  replaced by tiny vector **icons** (square, triangle, diamond, plus) in the owner's colour.
- Hit/cloak state machine: `update_static` (`src/game/motion.cpp` ~352).
- **Frame geometry is gameplay.** `calc_bounds()` (`src/game/motion.cpp` ~703) builds each
  object's collision bounds from its current frame's size and centre, so the `.pn` rectangles
  affect hits, not just drawing. Briefing maps and the minicomputer also size icons from them.
- **High-resolution sheets (`@2x`, `@4x`).** A sprite may have `image@2x.png`/`overlay@2x.png`
  and `@4x` versions next to the 1× sheets (`NatePixTable::NatePixTable`,
  `src/drawing/pix-table.cpp`).
  - Choice per effective draw scale, made when the level loads the sprite (`Pix::add`,
    `src/drawing/sprite-handling.cpp`).
    - The effective draw scale is the UI scale × the object's sprite scale (`sprite_scale`,
      rounded up). Capital ships (`tpl/capship`, `rotation: {scale: 2.0}`) count double.
    - Order: the smallest sheet at least as detailed as the screen, then larger, then smaller.
      1× → 1×; 2× → @2x, @4x, 1×; 3× → @4x, @2x, 1×; 4× and up → @4x, @2x, 1×. There's no `@3x`.
    - A capital ship therefore uses @2x at UI 1× and @4x from UI 2×.
    - A sprite first loaded for a smaller scale is reloaded with the sharper sheet. The cursor and
      other non-object sprites use the UI scale.
  - A sheet is used only if both its image and overlay exist and are exactly that multiple of the
    1× size; otherwise it's skipped with a message on stderr, and the next one is tried.
  - Frame rectangles and pivots stay in 1× units (`width()`, `height()`, `center()`), so collision
    bounds, weapon mounts and replays are unchanged. Only the pixels in `pix_map()` and the
    texture grow (`Frame::scale()`).
  - Memory grows with the square of the scale (16× per sprite at @4x), for each owner colour
    loaded.
- **Renderer fix that came with this:** `OpenGlTextureImpl::draw_internal`
  (`src/video/opengl-driver.cpp`) used to set texture coordinates to `size / scale`, so a whole
  texture with scale > 1 showed only its top-left part. It never showed up before, because
  fonts draw through `draw_quad` and there were no `@2x` pictures. It now covers the full texture.
- **Viewing a sprite in game without touching the save:** pass a scenario folder on the command
  line (`antares.exe … <folder>`). The folder needs only an `info.pn` with a different title,
  which gives it its own progress file, and a `levels/` folder; chapter 1 is always unlocked.
  Everything else falls back to the factory scenario folder and `data/`, so override sprites still
  apply. A one-level scenario with four Human cruisers was used to test `@2x`/`@4x` at UI 1×, 2×,
  3× and 4×. Only the flagship stayed in view in that test, for the reasons below.
- **Showing ships side by side in a test level** (2026-10-18). Ships don't sit where a level puts
  them:
  - Every ship starts at full speed in a random direction (`tpl/ship`: `initial_direction` 0–360,
    and the initial velocity is `max_velocity` unless the object sets `initial_velocity`;
    `src/game/space-object.cpp` ~257).
  - The camera follows the flagship, so the others are soon out of view.
  - The test scenario can also hold `objects/…` files, which take precedence over `data/`. Test
    copies with `initial_velocity: 0.0` and a fixed `initial_direction` (plus zero
    `max_velocity`/`thrust`/`turn_rate` for non-player ships) park every ship facing the same way.
  - Without an `angle`, a level also rotates all start positions randomly (`src/game/level.cpp`
    ~203).
  - Return in a mission opens the text entry, which then swallows keys such as F9/F10.
  - Synthetic key presses need hardware scan codes, or GLFW ignores function keys.
- **Capital ships are drawn at 2×.** `tpl/capship` sets `rotation: {scale: 2.0}`, so the carrier's
  1× sheet appears twice its size at 1:1 zoom (measured in game: 196 × 93 px for a 100 × 48 px
  frame).
- **Remastered Human cruiser in game** (2026-10-18): installed at 1×, 2× and 4× through the
  override folder and shown beside the original fighter and carrier in the test scenario, at UI 1×,
  2× and 4× and 1:1 and 1:2 zoom.
  - It draws on its pivot at its 1× frame size (40 × 25 px for the frame shown, the same as the
    original).
  - UI 2× and 4× use the @2x/@4x sheets.
  - Test files removed afterwards; the save was untouched (checked by hash).

### 4.3.1 Sprite render tools (`tools/sprite-render/`)

Not part of the game build. They need Blender 5.2 (tested with 5.2.2 at
`C:\Program Files\Blender Foundation\Blender 5.2\blender.exe`) and Python 3 with Pillow.
Output goes to `out/sprite-render/` (ignored by git); nothing is written to `data/`.

- `render_sprite.py` runs inside Blender and renders a model into a sprite's frame layout:

  ```bat
  blender -b --factory-startup [model.blend] -P tools/sprite-render/render_sprite.py -- --sprite uns/cruiser --placeholder cruiser --tilt 44
  ```

  - Reads the original `.pn` and keeps every frame's rectangle and pivot.
  - The model's origin must be at the pivot, with the nose along +Y. Materials named `team…`
    take the owner's colour.
  - Orthographic camera, tilted by `--tilt`; the sun is fixed relative to the camera.
  - Renders at 8 px per 1× px with EEVEE and box-filters down (premultiplied alpha). Writes
    `image`/`overlay` sheets at 1×, `@2x` and `@4x`, plus `fit-report.md`: per frame, how much of
    the ship falls outside the original rectangle (threshold 0.5 %).
  - About 17 s for the cruiser's 24 frames.
- `compare_sheet.py` (Pillow) makes `compare.png`: original vs new 1× vs overlay vs new 4×, with pivots marked.
- `override.py install --sprite <name> [--scales 1,2,4]` / `remove` / `status` copies the sheets
  (1× by default) into the
  factory scenario folder for an in-game test. A manifest records exactly which files and folders
  it created, and `remove` deletes only those. It refuses to overwrite files it didn't install.
- Placeholder Human cruiser (2026-10-18): fits all 24 frames with at most 4.2 % overflow (the
  placeholder is slimmer than the original, so some down-facing frames overflow at the sides).
  Tested in game in chapter 18 at 2:1 zoom: it draws aligned on its pivot, with team tint.
- **Model scripts** (`--model <script> --variant <name>`): `build(variant)` creates the ship and
  returns its settings:
  - `tilt`, and `scale` (pixels per unit, if the model is built in original-pixel units);
  - `camera_distance` (perspective camera; orthographic if absent) and `screen_offset`;
  - `lights` (key/rim/fill, defined relative to the camera) and `ambient`.

  Team materials can carry a `team_white` value node, so the overlay pass keeps their bump and
  roughness. Use `--engine CYCLES` for ray-traced AO and soft shadows (GPU with OptiX denoising).
  The fit report adds silhouette IoU against the original, per-edge spill and render time.
- `uns_cruiser.py` is the Human cruiser in three modernisation levels: `A` subtle, `B` balanced,
  `C` richest (`docs/art-direction.md`). All three overflow by at most 0.5 % (IoU about 0.79) and
  render in about 15 s each (24 frames × 2 passes, 1×/2×/4×, 128 samples, RTX 5090).
- `variant_sheet.py` (Pillow) compares an original with variants: a detail sheet at 2× and 4×,
  and a readability sheet at real 1× size, 1:2 zoom and team-tinted.

#### Remastering from the original frames (2026-10-18, Human cruiser trial)

The A/B/C variants drifted from the original design. The remaster instead takes the shape and
colours from the original's 24 frames. These tools need numpy, scipy and Pillow; the Blender
model only needs numpy.

- `fit_metric.py --sprite X --render DIR`: per frame and overall, compares the 1× silhouettes
  (alpha ≥ 50 %): IoU, extra/missing pixels, mean and 95th-percentile outline distance, centroid
  shift. Targets: IoU mean ≥ 0.88, worst ≥ 0.85, outline mean ≤ 0.5 px, worst p95 ≤ 1.5 px.
  At these sizes 1 px all round costs about 20 % IoU, so outline distance is the more intuitive
  number.
- `sprite_geometry.py`: the original camera as code. It projects model points into frame *i*
  (rotation clockwise from nose-up, tilt, perspective distance, per-frame pivot).
- `carve.py`: the visual hull. It keeps every voxel that falls inside the silhouette in (almost)
  all frames, then back-projects the original's colours and overlay onto the hull's surface.
  - For the cruiser (tilt 44°, distance 200) it reprojects at IoU 0.97 mean, 0.94 worst, outline
    0.12 px. That is the ceiling for any rigid shape, and it shows the frames are very consistent.
  - The hull itself is lumpy and **too tall**. Every frame looks down from the same 44°, so the
    outlines can't separate a section's height from its width (it comes out lens-shaped).
    Carving is used for measurement only.
- `uns_cruiser_shape.py` + `fit_shape.py` → `uns_cruiser_fit.json`: the hand-built parametric
  shape, fitted to the frames.
  - Parts: a lofted superellipse hull through 10 stations, two rounded-box pods, the engine block.
  - Powell's method on soft IoU at 3×3 sub-pixels, with a penalty for spilling out of the frame
    and a smoothness term.
  - Starting from the carve's measurements, it reaches 0.90 / 0.87 / 0.48 px in a few minutes.
- `uns_cruiser_remaster.py` (model script) builds clean meshes from that same shape.
  - Use `--engine CYCLES --samples 128`; about 60 s for 24 frames × 2 passes.
  - Blender's frames landed a constant 0.5 px right of the fitter's projection (pixel-centre
    convention), so the script sets `screen_offset (-0.5, 0)`.
  - Rendered fit: **IoU 0.91 mean, 0.88 worst, outline 0.38 px, p95 1.41 px**. That beats the
    numpy fit; the old variants scored 0.79 / 0.73 / 0.92 px.
- `calibrate_colours.py` back-projects the original and the render onto the same carved points
  and scales each region's linear base colour by original ÷ render (`uns_cruiser_colours.json`).
  Two or three rounds converge for hull (152 vs 156 sRGB) and pods (100 vs 100).
  - The 2–4 px-wide dorsal strip and the engine block stop responding. At 1× their pixels are
    mostly blended with the hull's, so they stay about 20 levels lighter than the original
    unless they are made pure black at 4×. They are left at sensible dark values.
- `remaster_sheet.py` makes the comparison sheet for all frames:
  - original vs remaster at UI 1×, 2× and 4× (original pixel-doubled);
  - a silhouette difference view;
  - per-frame and overall fit numbers.

What the cruiser's original frames show (colours back-projected in 3D):
- **Hull:** light grey, with a darker strip along the top centre from just behind the cockpit to
  the engine block. A dark cockpit window sits about 6–10 units behind the nose.
- **Pods:** mid grey, darker underneath and at the rear. Each pod's sides are bright at the top
  and bottom, with a dark groove between, so it is **two stacked nacelles**.
- **Block:** the engine block is dark.
- **Lights:** four red lights, one on the back of each nacelle (x ±6.1, z ≈ +0.6 and −7.5).
- **Team colour:** the overlay covers 100 % of the ship, lights included. The remaster keeps
  that, which differs from `docs/art-direction.md` §3 (glows outside the overlay).

The frames don't contain the following, so the remaster had to invent it:
- the cross-section's height-to-width ratio and the underside shape;
- the bevel radii and exact pod corner shape;
- the panel-line layout (the original has none legible);
- material roughness and metalness;
- the cockpit's glass look;
- the lights' shape and size.

#### Shared remaster tools and the next Human ships (2026-10-18)

The pipeline per ship:
1. `camera_sweep.py` finds the original's camera.
2. `carve.py` measures the layout, colour regions and lights.
3. A small hand-written `<ship>_shape.py` sets the layout, starting values and `look`.
4. `fit_shape.py` fits it.
5. A render, then `fit_metric.py --set-offset` stores the ship's sub-pixel offset.
6. `calibrate_colours.py` (two or three rounds).
7. `remaster_sheet.py`.

Two reusable shape kits cover the ships so far:
- `twin_pod.py` + `twin_pod_model.py`: hull loft, two engine pods, engine block. This is the
  cruiser's layout, generalised with per-ship stations, bounds and `look` (dorsal strip, cockpit,
  nacelle groove, one light per pod or per nacelle). Render with
  `--model tools/sprite-render/twin_pod_model.py --variant <ship>`.
- `box_kit.py` + `box_kit_model.py`: any list of fitted rounded boxes, which can be mirrored,
  repeated as segments (hangar modules) or chamfered at the front. Use it for layouts that aren't
  twin-pod.

Each ship's `<ship>_fit.json` stores its camera, including the measured screen offset. The offset
differs per ship (cruiser −0.5, fighter −0.69, carrier +0.04 px), so it is measured, not fixed.
Notes on the tools:
- `fit_shape.py --sub`: keep the grid step at most one sub-pixel, or frames seen straight on get
  empty stripes and fit badly.

Results:

| | Camera | Ceiling (carved) | Rendered remaster | Wall-clock | Per-ship code |
|---|---|---|---|---|---|
| `uns/fighter` | 44°, 200 | 0.91 / 0.875 / 0.24 px | IoU 0.90 / 0.87, outline 0.29 px, p95 1.0 px | 7 min | 30-line shape module |
| `uns/carrier` | 46°, 400 | 0.989 / 0.977 / 0.07 px | IoU 0.94 / 0.92, outline 0.46 px, p95 2.0 px | 12 min | 35-line shape module, plus the new box kit |

- The cameras support one renderer framing every ship: a tilt of 44–46°, with the distance
  growing with ship size.
- The carrier's p95 misses 1.5 px only in frame 0, where the original has a 1 px bow antenna.
- Compute per ship is about 2–4 minutes (sweep, carve, fit, about 8 renders of 10–30 s each). The
  rest is the hand-written shape module plus fixing what a new ship shows up.
- What the frames showed:
  - **Fighter:** cockpit, dark crossbar, one red light per pod; the pods are single units, with
    no nacelle groove.
  - **Carrier:** bright deck spine between two dark stripes, four hangar modules, a fin, one red
    light per pod.
  - **Both:** the team overlay covers the whole ship.

**Capital ships and @2x sheets.** Fixed 2026-10-18: sheets are now chosen by effective draw scale
(§4.3).
- In game, the remastered carrier at UI 1× draws from its @2x sheet: about a third of its 2×2
  pixel blocks are uniform, against nearly all for a stretched 1× sheet.
- **In-game test of all three** (2026-10-18): the remastered fighter, cruiser and carrier were
  shown beside the original gunship, heavy destroyer and transport at UI 1×, 2× and 4× and 1:1 and
  1:2 zoom. The test files were removed afterwards, and the save was unchanged (checked by hash).
- **Observation:** the carrier's flat, light deck reads brighter than the mottled originals around
  it, although its average tone matches its own original.
- The carrier's frame-0 miss (p95 2.0 px, from the original's 1 px bow antenna) is accepted.

### 4.4 Effects and other layers

Drawn each frame in this order (`GamePlay::draw`, `src/game/main.cpp` ~399):

1. **Starfield** (`src/game/starfield.cpp`): 125 parallax stars at three speeds/brightnesses,
   125 spark particles (explosion debris), and warp-streak lines when warping.
2. **Sector lines**: grid lines for spatial reference (`draw_sector_lines`).
3. **Vectors** (`src/game/vector.cpp`): **rays** (point-to-point beams, optional
   colour-cycling and **lightning** jitter) and **bolts** (moving line segments).
4. **Sprites** (4.3).
5. **Labels** (`src/game/labels.cpp`): object names, control/target labels, hotkey numbers.
6. **Messages** (`src/game/messages.cpp`).
7. **Site**: triangle showing the flagship's heading (`draw_site`).
8. **Instruments** (3.4).
9. **Cursor** sprite (only when the game card is on top).
10. **Hint line** (tutorial arrows/lines, `HintLine::draw`).
11. **Transitions** (`src/video/transitions.cpp`): full-screen colour flashes — on collisions
    (`non-player-ship.cpp` ~1345) and when a level script calls for one (`action.cpp` ~660).
    `ColorFade` cards fade between screens.

Text uses bitmap fonts `title`, `button`, `button-small`, `computer`, `tactical`
(`data/fonts/*.png` + `.pn` glyph tables; `src/drawing/text.cpp`). `StyledText`
(`src/drawing/styled-text.cpp`) lays out briefing and message text with inline pictures and a
"retro" typewriter effect.

---

## 5. Where to look

| Topic | Primary files |
|---|---|
| Key codes & defaults | `include/config/keys.hpp`, `src/config/preferences.cpp`, `src/config/file-prefs-driver.cpp` |
| GLFW input & window | `src/glfw/video-driver.cpp` |
| In-mission global keys, game loop, draw order | `src/game/main.cpp` |
| Ship keys, selection, mouse, gamepad | `src/game/player-ship.cpp`, `src/game/cursor.cpp` |
| Orders, build, minicomputer | `src/game/minicomputer.cpp`, `src/game/admiral.cpp` |
| Ship behaviour/physics | `src/game/non-player-ship.cpp`, `src/game/motion.cpp`, `src/game/action.cpp` |
| HUD | `src/game/instruments.cpp`, `src/game/messages.cpp`, `src/game/labels.cpp` |
| Screens | `src/ui/screens/*.cpp`, `src/ui/flows/*.cpp`, `data/interfaces/` |
| Widgets | `src/ui/screen.cpp`, `src/ui/widget.cpp`, `src/drawing/interface.cpp` |
| Rendering | `src/video/opengl-driver.cpp`, `src/video/glsl/`, `src/drawing/` |
| Sprites & art | `src/drawing/pix-table.cpp`, `src/drawing/sprite-handling.cpp`, `data/sprites/`, `data/pictures/` |
| Text | `src/drawing/text.cpp`, `src/drawing/styled-text.cpp`, `data/fonts/`, `data/strings/` |
