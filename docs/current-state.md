# Antares: how it works today

A snapshot of the game's controls, command model, UI and rendering as they exist in
the code at upstream `d8ce5826` (plus the local minimize fix, `651bb17e`). Paths are
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
The OS cursor is hidden; the game draws its own (`GLFW_CURSOR_HIDDEN`).

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
- Layouts are in a **640 × 480 coordinate space, centered** in the window
  (`InterfaceScreen::offset`, `screen.cpp` ~181). `fullscreen: true` clears the rest of the
  window to black; `false` overlays the card underneath.
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
| **Options › Video** | `options.cpp` ~231 | `interfaces/options/video.pn` | Fullscreen checkbox; Key Controls, Sound, Cancel, Done |
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

- **Play area** = window width − 128 − 32, full height; it grows with the window, so a bigger
  window shows more space (`play_screen()`, `viewport()`).
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
- **Resolution**: there is no fixed internal resolution. The logical screen size = window size
  (default 640 × 480, saved in prefs; fullscreen = monitor mode). Rendering uses an integer
  **scale** = framebuffer width / window width (`OpenGlVideoDriver::scale`,
  `src/video/opengl-driver.cpp` ~385) for HiDPI; only fonts ship `@2x` art (`data/fonts`).
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
- **Team colour**: `NatePixTable(name, hue)` (`src/drawing/pix-table.cpp` ~38) tints the
  overlay layer with the owner's hue and composites it onto the image; gray = untinted.
- Drawing: `draw_sprites()` (`src/drawing/sprite-handling.cpp` ~168) draws three layers in order
  **BASES → SHIPS → SHOTS**, scaled by zoom. Below 1:4 zoom (`kBlipThreshhold`), sprites are
  replaced by tiny vector **icons** (square, triangle, diamond, plus) in the owner's colour.
- Hit/cloak state machine: `update_static` (`src/game/motion.cpp` ~352).

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
