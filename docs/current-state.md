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
- **Actions.** The rebindable actions are the `KeyNum` enum (`include/config/keys.hpp`):
  - the original 44 (0–43), whose numbers replays and old configs store and which never change;
  - since 2026-10-20, two added after them: Attack move (44) and Patrol (45). They can be bound
    but do nothing until the Modern mode is built (`docs/modern-controls.md`).
- **Bindings** (since 2026-10-20, `include/config/bindings.hpp`). Each action has:
  - a **primary** and a **secondary** binding, each a key, a mouse button (middle and extra
    buttons) or a wheel direction;
  - a **gamepad** button.

  Some inputs can't be bound because the game uses them itself:
  - Esc, Return and Caps Lock;
  - the left and right mouse buttons (select and target);
  - the gamepad's LB and RB (the bumper commands) and Start (the Play Again menu).

  Clashes: two actions may not share an input, except the weapons (fire 1, fire 2, special), which
  may, because the Ares gamepad fires both weapons with RT.
- **Presets** (data tables in `src/config/bindings.cpp`):
  - **Classic** (the default) is the game as it was: the keys below, the wheel as the secondary
    binding of zoom in and out, and Ares' gamepad mapping (§1.4). Attack move and patrol are
    unbound. One exception: since 2026-10-20 the fire keys are X and Z, not Left Alt and the
    Windows key (§1.2).
  - **Modern: Ares** (Modern (Ares-style), the default Modern layout since 2026-10-20). The arrow
    keys steer, the PC equivalent of Ares' keypad 8-4-5-6. Every other fire and action key is under
    the left hand:
    - fire 1 Space, fire 2 F, special D, warp Tab;
    - select friendly Q, select hostile E, select base R; target Shift; order S;
    - zoom X and Z, as well as the wheel;
    - minicomputer T (up), G (down), B (accept), V (cancel); transfer C; next message `;
    - attack move A (A, then click, once the Modern mode exists), patrol W.

    The exceptions on the right hand are the zoom shortcuts (F9–F12, End, Home, Page Up, as
    Classic) and hotkeys 6–0. Ctrl, Alt and the Windows keys are left unbound: Ctrl will be the
    Modern group and attack-move modifier, and Alt+Tab, Alt+Enter and the Windows key belong to
    Windows. The tradeoff: the right hand can't steer and use the mouse at once; ships coast, and
    autopilot (Shift+Tab) flies the flagship while the hand is on the mouse.
  - **Modern: WASD** flies on W A S D, with:
    - fire 1 Space, fire 2 F, special R;
    - select friendly Q, select hostile E, select base B; order Ctrl;
    - zoom = and −, as well as the wheel;
    - attack move G, patrol T;
    - everything else as Classic.
  - All three presets bind the gamepad as Ares did.
  - The full key tables, with hands and clashes, are in `docs/modern-controls.md` §8.
  - A config saved with stage 4's single "modern" preset loads as Modern: WASD, the layout it had.
- **Where bindings are edited:** **Options › Key Controls**, the rebinding screen (2026-10-20, §3.3).
  - It shows every action's three bindings, a page at a time.
  - The preset choice (Preset: Classic / Preset: Modern) replaces every binding with that preset's,
    and "(CHANGED)" shows when the bindings differ from it.
  - **Reset to Defaults** restores the chosen preset. Cancel undoes any change.
- **Config file** (`src/config/file-prefs-driver.cpp`):
  - Since 2026-10-20 it is version 2: `version: 2` and a `controls` section with the preset and,
    for each action, `{primary, secondary, gamepad}`. A binding is written as `{key: 96}`,
    `{mouse: 2}`, `{wheel: "up"}` or `{gamepad: "rt"}`.
  - It still writes the version 1 `keys` section (each original action's first key, 0 for none),
    so an older build reads the keys.
  - A version 1 file (no `controls`) loads as Classic with its saved keys as the primary bindings,
    then has the old unreachable defaults moved (below). Invalid bindings in a file (a reserved
    input, a gamepad button in a key slot) are ignored.
- **How an input becomes an action.** `PlayerShip` looks up the actions bound to each key, extra
  mouse button, wheel notch or gamepad button (`Preferences`' `actions_for`) and presses them.
  - Each action counts how many of its inputs are held: it starts on the first press and ends on
    the last release.
  - `GamePlay` handles help, volume, music and fast motion the same way, from whatever they are
    bound to.
  - Everything that changes the game still reaches `PlayerShip` through the input queue at a major
    tick.
  - Replays are played back as their stored action numbers (`ActionDownEvent`/`ActionUpEvent`),
    not turned into the player's keys, so they play the same whatever the bindings are.
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
| Fire weapon 1 | X (Ares and Classic until 2026-10-20: Option, i.e. Left Alt) | Fire pulse weapon while held (`kPulseKey`) |
| Fire weapon 2 | Z (until 2026-10-20: Command, i.e. the left Windows key) | Fire beam weapon while held (`kBeamKey`) |
| Fire/activate special | Space | Special weapon/device while held (`kSpecialKey`) |
| Warp | Tab | Warp while held. **With Target (Shift) held: engage autopilot** toward the target |

**Command** (tab "Command")

| Action | Default key | What it does |
|---|---|---|
| Select friendly | Keypad . | Cycle the **control** selection (yellow label) through friendly ships, nearest in the direction your ship faces. With Target held: *target* a friendly instead |
| Select hostile | Keypad * | Always *targets* (sky-blue label) the next hostile in your facing direction |
| Select base | Keypad / | Select your next base as control object. With Target held: target any base (friend or foe) |
| Target | Shift | A modifier: makes select keys, hotkeys, Warp and mouse clicks act on the **target** instead of the control object. Tapped and held ≥ 0.75 s alone, then released: **targets your own ship** |
| Order to go | Control | Orders the **control** object to go to the current **target** (`kGiveCommandKey` → `PlayerShipGiveCommand`) |
| Scale in / out | Keypad + / Keypad − | Step zoom in / out one level (the mouse wheel does too, see §1.3) |
| Computer previous / next | Up / Down arrow | Move the minicomputer's line cursor |
| Computer accept | Right arrow | Activate the highlighted minicomputer line |
| Computer cancel | Left arrow | Back out to the minicomputer main menu |

**Shortcuts** (tab "Shortcuts")

| Action | Default key | What it does |
|---|---|---|
| Transfer control | F8 | Switch piloting to the control object (must be your own thinking ship) |
| Zoom to 1:1 / 1:2 / 1:4 / 1:16 | F9 / F10 / F11 / F12 | Jump to that zoom; pressing the same key again returns to the previous zoom |
| Zoom to closest hostile | End | Auto-scale to fit nearest foe |
| Zoom to closest object | Home | Auto-scale to fit nearest object |
| Zoom to all | Page Up | Fit everything |
| Message next page/clear | Backspace | Advance/clear the on-screen message |

**Utility** (tab "Utility")

| Action | Default key | What it does |
|---|---|---|
| Help | F1 | Opens the in-game Help screen (pauses) |
| Lower volume | F2 | Volume −1 (down to 0; fixed 2026-10-20, it used to do nothing) |
| Raise volume | F3 | Volume +1, up to 8 (capped 2026-10-20; earlier builds let it pass 8, and a saved volume outside 0–8 is now brought back into range on load, `file-prefs-driver.cpp`) |
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

> **PC defaults (2026-10-20).** Ares' defaults for Select friendly (Keypad Clear), Select hostile
> (Keypad =) and Zoom to closest hostile (Help) don't exist on PC keyboards (GLFW has no key for
> Clear or Help; most PC keypads have no `=`). The defaults are now Keypad `.`, Keypad `*` and End.
> A saved `config.pn` still holding one of the old keys is moved to the new one when loaded, unless
> another action already uses it (`migrate_unreachable_defaults`, `src/config/preferences.cpp`);
> every other saved binding is kept. `prefs-test` covers the defaults and the move.

**Classic's fire keys and Windows (tested 2026-10-20, in a level, windowed and fullscreen; keys
sent as real key presses).** Both break play:
- **Left Alt (fire 1) held, then Tab (warp):**
  - While Alt was held, Windows' Task Switching opened.
  - On release, Windows switched to another program, and the game lost focus.
  - Fullscreen, the game minimized as soon as the switcher opened.
- **Left Windows key (fire 2):**
  - Nothing happened while it was held.
  - On release, the Start menu (Search) opened and took focus.
  - Fullscreen, the game minimized.

**Changed 2026-10-20:** Classic's fire 1 is now X and fire 2 Z, the keys just above Left Alt and
the Windows key. Neither was used by another Classic action, and no preset binds Alt or the Windows
keys any more.

Saved configs move to the new keys only where both hold:
- fire 1 is still on Left Alt, or fire 2 still on the left Windows key;
- the new key isn't used by another action.

This applies to configs written before the change (version 1 and 2) that are on the Classic preset
(`migrate_classic_fire_keys`, `src/config/preferences.cpp`). Saves are now version 3, so a key chosen
afterwards is never moved.

**Checked in game (2026-10-20):**
- Key Controls shows X and Z at UI 1×, 2× and 4×.
- In a level, X, Z and Tab held together (both weapons while warping) kept the game in the
  foreground and never minimized it: windowed at 1×, 2× and 4×, and fullscreen.
- At 2× the pulse and the beam both fired as the warp started.
- A config still on Left Alt and the Windows key was saved back with X and Z.

Tutorial messages name the player's keys. The `replay` test tool keeps Ares' Option and Command for
the fire keys, so its output matches the recorded test data (`src/bin/replay.cpp`).

### 1.3 Mouse

| Input | Where | Effect |
|---|---|---|
| Move | mission | Shows a crosshair cursor sprite; it hides after 1 s idle (`src/game/cursor.cpp`, `kTimeout`). While the cursor is "active", keyboard select/target keys are ignored (`PlayerShip::update`) |
| Left click | play area | Select the friendly object under the cursor (±16 px) as **control** (`PlayerShipHandleClick`, `player-ship.cpp` ~1025) |
| Right click, or left click with Target held | play area | **Target** the object under the cursor (friend or foe) |
| Click / double-click | minicomputer | Highlight a line / execute it (`MiniComputerHandleClick`, `…DoubleClick`) |
| Click | menus | Press buttons, checkboxes, tabs (`src/ui/screen.cpp`, `src/ui/widget.cpp`) |
| Any click / key | intros, fades, scroll text | Skip |

Double-click window is 30 ticks (0.5 s, `kDoubleClickInterval`).

**Mouse wheel (2026-10-20):** zooms in a mission. Up steps toward 2:1, down toward 1:16 and stops
there (unlike the zoom keys it doesn't step into the automatic zooms); from an automatic zoom (nearest
foe or object, all), up goes to 1:16. The GLFW driver turns scrolling into whole notches
(`GLFWVideoDriver::scroll`; touchpad fractions add up), sends a `MouseWheelEvent`, and `GamePlay`
queues it in the input source like a key, so it reaches `PlayerShip` at a major tick and becomes the
same `ZOOM_IN`/`ZOOM_OUT` player event as the zoom keys (marked as the wheel's) — deterministic, and
`ZoomCondition`s fire as for the keys. Levels' zoom-in/zoom-out masks apply to it; the mouse mask
doesn't (tutorial 1 enables zooming while the mouse is still off). Replays don't record or react to
it; no action numbers changed. The rule is `wheel_zoom()` (`player-ship.cpp`), tested by
`controls-test`. Menus ignore the wheel.
The OS cursor is hidden; the game draws its own (`GLFW_CURSOR_HIDDEN`). All mouse positions are
in logical pixels: the driver converts window coordinates through the UI scale and border
(`GLFWVideoDriver::to_screen`), so clicks stay exact at every scale.

### 1.4 Gamepad (summary)

**Bindings (2026-10-20).** The gamepad's simple buttons are now bindings (the gamepad slot of each
action, §1.1), so they can be rebound. Classic and Modern both bind them as Ares did:
- A thrust, B brake, RT fires weapons 1 and 2, LT special;
- the left stick button warps; X and Y zoom out and in; Back is next message;
- the D-pad drives the minicomputer.

A pad button bound to warp toggles it, as before: held until the warp starts, it then stays on
until pressed again.

The left stick (steering), the bumpers and their commands, and Start stay fixed.

Two side effects of routing pad buttons through actions:
- levels' key masks now apply to them as to the keys (before, the pad bypassed them);
- turning warp off no longer also drops the other held pad buttons. Before, a slip (`&= !kWarpKey`)
  cleared them all.

**Platforms.** Until 2026-10-20 only the macOS driver read a gamepad (`src/mac/video-driver.cpp`,
Joy-Con); on Windows no gamepad input arrived. The GLFW driver now polls the first connected
gamepad with a standard mapping (XInput pads and others GLFW knows) once a frame
(`GLFWVideoDriver::poll_gamepad`) and `GamepadPoller` (`src/ui/gamepad-poller.cpp`) sends the same
gamepad events: buttons as they change; triggers as LT/RT (pressed above 75 %, released below 40 %);
the left stick as one of 32 directions or centred (dead zone 0.6 out, 0.45 back), only on change,
as the vector `PlayerShip::gamepad_stick` expects (it adds 180°). A disconnect releases everything
held. Gamepad input switches on-screen labels to gamepad names; a key or click switches back.
`controls-test` covers the mapping and checks that each stick direction steers the flagship that
way. Not yet tried with a real controller (none was connected when it was written).

**Controller detection on Windows (2026-10-21, `docs/startup-timing.md`).** GLFW is built from
`BUILD.gn`'s `:libglfw-win`, with an altered copy of its joystick file
(`src/glfw/win32-joystick.c`); the `ext/glfw` submodule is unchanged.
- `glfwInit()` and device arrival detect XInput pads only, which is instant.
- The DirectInput scan, which can stall for many seconds on some HID devices, runs on a worker
  thread. It starts when the main menu first shows (`VideoDriver::start_controller_scan`).
- `poll_gamepad` opens the devices the scan found, on the main thread.
- So a DirectInput-only pad works from a moment after the main menu, and a stalled scan never
  blocks the game.
- `--timing` marks: `sound init done`, `glfwInit done`, `window created`, `controller scan start`
  and `controller scan done` (with its time in ms).

Handled in `PlayerShip::gamepad_*` (`player-ship.cpp` ~519–692) and `GamePlay::gamepad_button_down`.
Left stick steers (turns toward stick direction); A thrust, B brake, RT fire 1+2, LT special,
LSB warp toggle, X/Y zoom out/in, Back next message, D-pad drives the minicomputer, Start opens
the Play Again menu. **LB** (target bumper) / **RB** (select bumper) turn A/B/X/Y/LSB into
select/target/order/autopilot/transfer commands aimed with the left stick.
Labels: `data/strings/gamepad/*.pn`.

**Slow start-up on some PCs (finding, 2026-10-20).**
- GLFW 3.3.2 enumerates DirectInput game controllers inside `glfwInit` (`_glfwInitJoysticksWin32`),
  before Antares creates its window. That has always been so; it isn't part of the stage 3 polling.
- DirectInput asks every HID device for its name. On the developer's PC a SteelSeries Apex 5
  keyboard (USB 1038:1650) takes 5 s to answer each of four interfaces, so the window appears only
  after about 30 s.
- Measured 2026-10-20, fullscreen, 3 launches each: 30.5–30.7 s to the window both with the build
  from `d3515022` (before the gamepad polling) and with `d27574a2` (with it).
- A plain DirectInput enumeration outside the game took 30.2 s and found no controllers.
- This depends on the device and its software, not on Antares code, so it was left alone.

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

### 2.4a Modern right-click orders (2026-10-20)

The first Modern feature (`src/game/modern-orders.cpp`; plan: `docs/modern-controls.md` step B).

**When it applies:**
- The Modern mode is on when a Modern preset (Modern: Ares or Modern: WASD) is chosen.
- It is decided when a level starts playing (`g.modern_controls`, `MainPlay`).
- It is always off in replays, the demo and the tutorials (levels with a skip option). Their text
  and checks teach the Classic keys.
- With it off, nothing differs from Classic.

**Selecting.** A left-click on a friendly ship selects it, as in Classic: it becomes the control
object, which will be the main ship once several can be selected. A click on empty space changes
nothing. In the Modern mode the selected ship gets yellow corner brackets, at least 12 px across,
so a blip at 1:16 is marked too.

**Right-click on the play area.** It first targets what is under the cursor (±16 px, as a Classic
right-click), then orders the selected ship:

| Under the cursor | Order | Engine |
|---|---|---|
| An enemy ship or base (anything not the player's) | Attack | The Classic order to the target: `destObject`, assault duty |
| A friendly ship | Follow (escort) | The same, escort duty: the ship matches the leader's heading and warps with it |
| A friendly base | Guard | The same, guard duty |
| Empty space | Move there | A point destination (`destinationLocation`, no `destObject`) with `timeFromOrigin` started at 15 s |

- **The click point:** converted to world coordinates at its major tick with the inverse of
  `scale_to_viewport` (`viewport_to_world`), so it is right at every zoom and UI scale.
- **When there's no order:** if the flagship, a base or a ship that can't take orders
  (`kStaticDestination`, or no `kCanAcceptDestination`) is selected, a right-click only targets.
  The flagship is never moved by these orders.
- **Feedback:** the order sound plays, and a ring in the order's colour closes in over a second
  where the order was given: red attack, aqua follow, gold guard, green move.
- **Cursor:** while hovering, the crosshair takes the colour of what a right-click would do.
- **Masks:** a level's mouse mask disables the right-click, and its "order" mask turns it into
  target-only.

**What the engine stores.** Nothing new. An object order is the existing destination object with
its duty. A move is a destination point with the 15 s clock started, so the ship flies straight
there, fights back if hit, and holds there on arrival (§1.3 of `docs/modern-controls.md`). A level's
scripted `target` action therefore replaces a Modern order exactly as it replaces a Classic one.
In the campaign, scripted orders go to ships whose destination the level locks, and those can't
take Modern orders at all.

**Input and determinism.** Clicks go through the per-tick input queue like every click. They are
resolved at the tick against that tick's sprite positions, as Classic clicks are.

**Diagnostics.** With `--timing`, each Modern order is written to the timing log. So are the
selected ship's screen position and the positions of the ships the player could order, four times
a second. The in-game check uses these to click on them.

**Tests:**
- **Unit tests (`controls-test`):** what a right-click does for each case, and screen→world→screen
  at nine zooms and three play-area origins (exact at 1:1 and further out, within 1 px zoomed in).
- **Replay test (`modern-orders`, a 17th replay test):** the `shoplifter-1` replay (ch04) played
  with `--modern` and input injected from `test-starbound/modern-orders.inject`. It runs these
  steps:
  - Select friendly, then right-clicks that only target while the Liner is selected;
  - a move by the Transport;
  - follow the flagship;
  - at zoom-to-all, a right-click on a Gaitori cruiser's position (attack);
  - a level-style scripted order replacing the Modern one.

  `orders.log` records each step. The expected output lives in `test-starbound/`, not upstream's
  `test/`. The `replay` tool's `--modern` and `--inject=FILE` options make this possible (see
  `ReplayMaster::inject`).

**In game (2026-10-20, ch05, Modern: Ares, windowed):** at UI 1×, 2× and 3× on a 5120×1440 client,
and 4× on 5120×1920:
- A left-click on an Engineer at zoom-to-all selected it.
- A right-click on empty space moved it, and a right-click on the flagship made it follow.
- Left-clicks on the flagship at 1:4 and 1:16 selected it.
- At UI 1×, left-clicks on the moving Engineer at 1:4 and 1:16 selected it. At 2×–4× it was off the
  play area at those zooms.
- The brackets, the green move cursor and the order ring showed at every scale.

**Limit (not new).** Select friendly (the key) only finds friendly ships within about 30° of the
flagship's heading, in Classic as in Modern. Clicking reaches any ship on the screen.

### 2.4b Modern multi-select (2026-10-20)

Step D of `docs/modern-controls.md`, in the Modern mode only (§2.4a); Classic, replays, the demo
and the tutorials are unchanged.

**The selection** (`g.modern_selection`) is a list of the player's ships, in selection order. Its
first ship is the admiral's control object, so the CONTROL line, Build and the rest still see one
ship.
- **Who can be selected:** only ships that can take orders: the player's own, not the flagship, not
  a base, not a ship whose destination the level locks (`group_selectable`).
- **Ships drop out** when they die, are captured or become locked.
- **When the control object changes some other way** (a click on one ship, Select friendly, a
  hotkey, a transfer), the selection becomes just that ship, or empty.

**Left button (play area):**
- **Click** on a ship: selects it, replacing the selection (as Classic); on empty space it changes
  nothing.
- **Shift+click** on one of the player's ships: adds it, and doesn't target it.
- **Drag** (more than 4 px): a thin yellow box while the button is held. On release, the player's
  orderable ships inside it are selected, in object order; the first becomes the control object.
  - **Shift+drag** adds them to the selection.
  - **A box with none inside** changes nothing.
- The drag is resolved from the queued mouse-down and mouse-up points, at the release's tick.

**Right-click orders** apply to every selected ship, under the rules of §2.4a (attack, follow,
guard, move). A ship is never ordered to itself.
- **Group moves:** the ships spread on a small centred grid around the point, 160 world units apart
  (`formation_offset`: 2 ships side by side, 3–4 in a 2×2, 5–9 in a 3×3), in selection order, so
  the result is deterministic.
- **Orders stored:** the same as before, the existing destinations only, so level scripts still
  override.

**Brackets** go on every selected ship and on the control object (also when it is a base or the
flagship); the control object's are brighter.

**Tests:**
- `controls-test`: who a box selects, the box test either way round, and the formation offsets
  (distinct, at least 160 apart, centred, deterministic).
- **Replay test `modern-multi`:** `space-race` played with `--modern` and
  `test-starbound/modern-multi.inject`. It runs these steps, and two runs are identical:
  - a drag box selects one ship;
  - a Shift+drag adds two more;
  - a group move (three grid points);
  - at zoom to all, a group attack on a Gaitori cruiser.
- **Why `space-race`:** `shoplifter-1` (the `modern-orders` test, unchanged) has only one ship the
  player can order, so it can't hold a multi-ship drag.
- **New `replay` tool inject steps:** `ldrag`, `keydown`, `keyup`, `log-orderable` and
  `log-selection`.

**In game (ch05, Modern: Ares, windowed, UI 2× and 4×):** checked from the `--timing` log, no
screenshots.
- At zoom to all, a drag box around both Engineers selected both.
- A right-click on empty space gave each a move to its own grid point.
- A right-click on the flagship made both follow it.

### 2.4c Modern control groups (2026-10-20)

Step E of `docs/modern-controls.md`, in the Modern mode only. Classic keeps its hotkeys, and
replays, the demo and the tutorials are unchanged.

**What the keys did before (checked first).** These bindings are the same in all three presets:
- **Numbers 1–0:** the hotkey actions 34–43. A tap selects the hotkey's object (or targets it with
  Shift); holding 0.85 s assigns it.
- **Shift:** the target modifier, so Shift+number targeted.
- **Ctrl:** in Classic and Modern: WASD it is the Order key and fired on key-down, so Ctrl+number
  would also give an order. Modern: Ares leaves Ctrl unbound.
- **Rebinding screen:** the numbers, Ctrl and Shift can all be rebound. Ctrl as the group modifier
  is a fixed key, not an action.

**How the clashes were resolved** (the approved plan, §5.1 of `modern-controls.md`):
- In the Modern mode the number actions are control groups, not hotkeys.
- In the Modern mode the Order key fires on release, and only if no number was pressed while it was
  held. This happens whatever key Order is bound to.

**The keys in the Modern mode:**
- **Ctrl+number:** saves the selection as that group. An empty selection clears the group.
- **Number:** selects the group's ships that can still take orders; the first becomes the control
  object. An empty group sounds a warning and changes nothing.
- **Shift+number:** adds the group to the selection. Here Shift is only a modifier, so no "target
  own ship".
- **The same number twice within 0.5 s:** shows a pointer for 1.5 s. If the group is on screen it
  is brackets around its middle; otherwise a line from the middle of the play area toward it. The
  view stays on the flagship (`docs/design.md`).

**Storage and determinism:**
- Groups are kept in the game state (`g.modern_groups`, `src/game/control-groups.cpp`): ten lists
  of object numbers and ids. They clear at level start.
- Ships that die, are captured or become locked drop out when a group is read, for good; a reused
  object slot doesn't come back as a different ship.
- Everything goes through the input queue as key events. There are no new order types.
- Ctrl is tracked as the raw key, so it works whatever it is bound to.

**Tests:**
- `controls-test`: save (no duplicates, replace, clear), recall, out-of-range groups, add (in
  order, nothing twice), dropping lost ships for good, reused slots, and clear.
- `modern-multi` replay test, extended:
  - Ctrl (also the replay tool's Order key) + 1 saves three ships, and no order is given;
  - selecting another ship, then 1 recalls the three, and a right-click moves them;
  - Shift+1 adds them to a single selected ship, giving four.

  Two runs are identical.
- `modern-orders` expected output changed in its screenshots only (its orders log is unchanged).
  The `shoplifter-1` recording presses Order at tick 297 and later, and in the Modern mode that
  order now comes on release.
- New `replay` tool inject steps: `keyevent-down`, `keyevent-up` (raw keys).

**In game (ch05, Modern: Ares, windowed, UI 2× and 4×, from the timing log):** these all passed:
- the drag selected both Engineers;
- Ctrl+1 saved them;
- clicking the flagship emptied the selection;
- 1 recalled both, and a right-click moved both;
- after clicking one Engineer, Shift+1 added the other.

The double-tap check failed only because the test pressed the keys about 560 ms apart, outside the
window. The window was then widened from 0.4 to 0.5 s, and the replay tool confirmed the pointer.

### 2.4d Modern attack-move and patrol (2026-10-21)

Steps C and F of `docs/modern-controls.md` (plan in §5b), in the Modern mode only. Both are built
from the engine's existing point order (`SetObjectLocationDestination`); there is no new order
type and no AI change.

**Keys** (rebindable actions 44 and 45, `docs/modern-controls.md` §8):
- **Attack-move:** A (Modern: Ares) or G (Modern: WASD) arms it; the next left-click in the play
  area gives it. **Ctrl+right-click** gives it at once (Ctrl as the raw key; Order's release then
  gives nothing).
- **Patrol:** W (Ares) or T (WASD) arms it; the next left-click gives it.
- While armed the crosshair is red (attack-move) or sky blue (patrol). A right-click disarms
  without an order. A level that disables orders (`kOrderKey` masked) disables both.

**Attack-move:** each selected ship gets a point order to its grid formation point (the
group-move grid) with the 15 s engage clock left at 0, the Classic Hold Position order. So it
turns aside to fight enemies within engage range, then carries on. A click on an enemy gives the
normal attack order instead. A plain Modern move still sets the clock to 15 s and doesn't turn
aside.

**Patrol:** each selected ship gets the same point order to its formation point B and a patrol
record (object number, id, A = where it was, B, which end it heads for) in `g.modern_patrols`.
`modern_patrol_step()` (`src/game/modern-orders.cpp`) runs once a major tick right after the
player's input (`src/game/main.cpp`):
- within max(the ship's arrive distance, 200 units) of the end it heads for → a point order to the
  other end (this also restarts the engage clock);
- its destination isn't that point any more (any other order, a script's order) → the record is
  dropped;
- dead, captured, locked or a reused object slot → the record is dropped.
- Records are dropped when the ship gets any other Modern order, and clear at level start.
- Selected patrolling ships show their route as a dashed sky-blue line.
Patrolling ships engage enemies on the way like attack-move. Game state only, record order, no
wall time: deterministic.

**Tests:**
- `controls-test` `PatrolStep`: keep, switch (least 200 units, the ship's larger arrive distance),
  drop when the destination changed, and no overflow on far coordinates.
- `modern-multi` replay test, extended: the attack-move key + left-click (four ships), Ctrl+right-
  click, patrol key + right-click (disarmed, no order), and the patrol key + left-click on group 1;
  the Heavy Cruiser then switches legs seven times. Two runs are identical; `modern-orders` is
  unchanged.

**In game (ch05, Modern: Ares, windowed, UI 2× and 4×, from the timing log):** all passed: the
drag selected both Engineers; A + click and Ctrl+right-click each gave two attack-moves (no other
order); W + right-click gave no order; W + click started two patrols and a leg switch followed.

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
| **Options › Video** | `options.cpp` ~231 | `interfaces/options/video.pn` + code | Fullscreen checkbox (applies next launch, or Alt+Enter); a column of **UI Scale** choices — Auto, 1x, 2x, 3x, 4x — that behave as radio buttons and apply immediately (added in code, `kUiScaleChoices`); below Fullscreen, **Graphics: Enhanced / Original** radio choices (`kGraphicsChoices`, saved as `enhanced graphics` in `config.pn`, default Enhanced; takes effect from the next level); Key Controls, Sound, Cancel (reverts), Done |
| **Options › Key Controls** (rebinding screen, 2026-10-20) | `options.cpp` (`KeyControlScreen`) | `interfaces/options/keys.pn` for the frame, Cancel, Options, Done and the conflict line; the rest in code (the data's tab box and instructions are removed with `InterfaceScreen::remove_widget`) | Page choices (Ship, Command, Shortcuts, Utility, Hot Keys, Modern: `kActionGroups`); preset choice with "(CHANGED)"; for each action on the page, Primary, Secondary and Gamepad cells. Click a cell, then press a key, the middle or a side mouse button, the wheel (primary and secondary) or a gamepad button (gamepad cell); Esc cancels; an input the slot doesn't take, or a reserved one, sounds a warning. **Clear** empties the selected cell. Clashing cells flash gold, the conflict line names the first pair, and Done and Options wait until they are fixed. **Reset to Defaults** restores the chosen preset. Cell changes are rebuilt from the screen's timer, not from the clicked cell's own action |

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
  - The game uses `art/` next to the data folder when it exists and **Graphics** in Video
    options is Enhanced (the default). Graphics: Original hides the folder (`art_path()` returns
    empty, `src/config/dirs.cpp`). Sprites load when a level starts, so a change shows from the
    next level; Options opens only from the main menu.
  - `--art DIR` points elsewhere and overrides Graphics: Original; `--no-art` always shows the
    originals (`configure_art()`, called from `src/win/main.cpp`).
  - `art-test` (unit test) checks where a sheet resolves (`resource_source()`) in each mode:
    Enhanced, Original, `--no-art`, `--art DIR`, and `--art` with Original.
  - Tested in game (2026-10-19, UI 2x, parked Human cruiser and Ishiman heavy destroyer):
    Original draws the original sprites and Enhanced the `art/` sheets. Clicking Graphics:
    Original then Done saves `false`; clicking Enhanced then Cancel puts `false` back.
  - Tests and tools never set it, so they always see the original art.
  - `scripts/dist.py win zip` packages `art/` next to `data/`, plus `COPYING`, `COPYING.LESSER`,
    `ASSETS.md` and `CREDITS.md` next to `Antares.exe`. Checked 2026-10-19 from a fresh folder with
    no config: it starts, and Video options shows Graphics: Enhanced by default.
  - Each file is looked up on its own, so a sprite takes its sheets from `art/` and its frame table
    from `data/`.
  - **Pictures and fonts** (`load_hidpi_texture`, `src/data/resource.cpp`) look for 1x, `@2x` and
    `@4x` files in the same search order (2026-10-19). Pictures take the smallest one at least as
    detailed as the screen, then larger, then smaller (UI 3x -> `@4x`, `@2x`, 1x), like sprite
    sheets. Fonts are pixel art and take one that divides the screen scale (UI 3x -> 1x drawn
    three times). A `@2x`/`@4x` file that isn't exactly 2x/4x its 1x size is skipped with a
    warning. A texture is laid out at its 1x size whatever its resolution
    (`OpenGlTextureImpl::size`).
  - **Filtering** (`TextureFilter`, `include/video/driver.hpp`): 1x textures and fonts are drawn
    nearest-neighbour, as always. Hi-res pictures are smoothed when drawn at another scale (a
    `@4x` picture at UI 3x); hi-res sprite sheets are smoothed when shrunk, with colour bled into
    clear texels so edges don't darken.
  - **Reloading:** fonts, HUD panels, the cursor and the splash and starmap pictures are loaded at
    start-up; `refresh_scaled_textures()` (`src/game/sys.cpp`, called every frame) reloads them
    when the UI scale or Graphics setting changes, and interface pictures reload on their next draw.
  - `art-test` also checks the picture and font choice at each scale, a wrong-size `@4x`, and that
    Graphics: Original loads only `data/` pictures.
  - Hi-res sheets and pictures read the 1x size from the PNG header; hi-res sprite frames free
    their CPU pixel copy once the texture is uploaded (measured in `docs/ui-graphics-upscale.md` §8).
  - `--timing FILE` (diagnostic) logs start-up milestones, level load start/done, every image
    decode (path, bytes, pixels, ms) and private memory (`src/game/timing.cpp`).
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
  twin-pod. Added 2026-10-19:
  - parts can round their front in plan view (`round`);
  - per-ship `look` settings:
    - `inset` (render-only shrink, so a dark `gap` core shows between blocks);
    - `tone` (panel or per-part brightness variation);
    - `metallic`;
    - `render` (per-ship lights);
    - `label_plan` (colour regions by plan view, for overhead cameras);
  - `gap` and `fitting` materials.
- `smooth_kit.py` + `smooth_kit_model.py` (2026-10-19): superellipsoid parts at any yaw (hubs,
  pods on angled struts). It fitted the Ishiman outlines well but looked soft. Use it only for
  genuinely round parts.
- `appearance_metric.py` (2026-10-19): the counterpart of `fit_metric.py` for looks.
  - Per region: mean colour and brightness spread, back-projected onto the carve.
  - Per frame at 1×: SSIM, luminance difference and contrast.
  - Screen-space brightness can differ from the region averages: the remastered Human cruiser
    averages 106 on screen against the original's 78, because the original has more dark pixels
    (shadowed sides and fine detail).

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

#### Upscaled sprite sheets in `art/` (2026-10-19)

63 ship sprites have @2x/@4x sheets in `art/`. They were upscaled from the original pixels
following `docs/upscale-decisions.md`:
- 61 use a Real-ESRGAN image with a tolerant-Scale2x overlay. On 2026-10-19 every ship was made
  enhanced, so the 11 that used Scale2x for the image switched to the AI image (including the
  Ishiman heavy cruiser and transport, which show cyan streaks under a team tint); the review
  marks 9 of them as clearly worse in some way (`docs/upscale-decisions.md`, ⚠).
- 2 use tolerant Scale2x for both: the Audemedon assault transport and the Cantharan gateship
  (the AI image showed streaks and squiggles).

**Non-ship graphics (2026-10-19):** 21 pictures (the title splash, with its faint stars blended
back from the original; the four main-menu room pieces; the intro starfields `log/starfield/a-c`
and stills `log/ares-departs`, `log/ish-encounter`; six race pictures; five briefing pictures) and 36 sprites (asteroids,
stations and level objects, 15 planets) also have Real-ESRGAN `@2x`/`@4x` files in `art/`; the
review and the exclusions are in `docs/ui-graphics-upscale.md`.

Of the other sprites, 12 stay original and 3 are the Human 3D remasters. Frame tables, pivots and
1x overlays come from `data/`.

**Enhanced at UI 1x (2026-10-19).** At UI 1x a normal ship draws its 1x sheet, so before this only
capital ships (drawn at 2x) and the Human 3D ships (which have their own 1x sheets) changed.
All 61 AI-image sprites now also have a 1x `image.png` in `art/`, shrunk from the AI @2x sheet
(`tools/sprite-render/shrink_ai_1x.py`; premultiplied 2x2 average with the original outline); the
overlay stays the original 1x. Eight of them were first kept original at 1x because the shrunk
image lost small lights or structure; they were installed anyway so that every ship is enhanced
(listed with ⚠ in `docs/upscale-decisions.md`). The two Scale2x sprites and the original ones are
unchanged at 1x. Checked in `ch15` at UI 1x with a test config: with Enhanced the player's Ishiman heavy
destroyer matches the new 1x sheet (rms 11.9 against 20.8 for the original), with Original the
`data/` sheet. In `ch15` this changes nine sprites at UI 1x: `aud/fighter`, `ish/aslttran`,
`ish/fighter`, `ish/gunship`, `ish/hvd`, `sal/aslttran`, `sal/cruiser`, `sal/gunship`,
`sal/transport`. `art/` is about 75 MB on disk (mostly @4x; the 1x images add 3 MB), stored with Git LFS.

In-game check (save-free test scenario, every ship of each faction parked, UI 1x-4x, 1:1 and 1:2
zoom, with and without `--no-art`):
- **Placement:** no ship is misplaced (largest shift under 1 logical px where the automatic
  comparison is reliable) and none is clipped.
- **Unreliable comparisons, checked by eye:** the comparison misreads the overlapping Cantharan
  gateship and the near-black Salrilian ships; both were checked visually.
- **One-ship levels:** a test level with a single ship ends straight away, so the Bazidan
  battleship and the border drone were tested together.
- **Largest campaign level** (`ch15`, UI 4x):
  - private memory 273 MB with `art/` against 215 MB without;
  - load (Begin to briefing) about 2.1 s against 1.9 s.
- **Tinted level** (`dev1`, salmon side, UI 2x):
  - private memory 279 MB against 223 MB;
  - load about 2.0 s against 1.7 s.
- **Tinted ships, zoomed in** (parked test scenario with a salmon player, 1:1 zoom, UI 2x and 4x,
  with and without `art/`):
  - most tinted ships look like the originals with smoother edges, because the overlay
    (tolerant Scale2x) is what the tint draws;
  - the Ishiman heavy cruiser and transport showed cyan streaks through the tint: their overlays
    cover only part of the ship, so the AI image showed through. Both now use tolerant Scale2x
    for the image too; rechecked under a salmon tint at UI 2x, the streaks are gone;
  - the near-black Salrilian heavy destroyer draws untinted with or without `art/`.
- **How much play is tinted:** players without a `hue:` use gray, which draws `image.png`. None of
  the 23 story levels (`tut1`-`tut3`, `ch01`-`ch20`) or the net levels set a hue; only `dev1`
  (one CPU side, salmon) does. So almost all play shows the image layer, not the overlay.

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
