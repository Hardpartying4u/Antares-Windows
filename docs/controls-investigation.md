# Controls investigation for M2 (2026-10-19)

Read-only investigation of the input system, before any M2 code. Code references are to the
current `windows-working-build` branch. `docs/current-state.md` §1–2 already lists the default
bindings; this document checks them against the code, fills the gaps and proposes the M2 plan.

## 1. How input reaches the game

```
GLFW callbacks (src/glfw/video-driver.cpp): key, char, mouse button, cursor position, window size
  -> KeyDownEvent / KeyUpEvent / MouseDownEvent / MouseUpEvent / MouseMoveEvent
  -> top Card on the stack (menus: src/ui/screen.cpp, src/ui/widget.cpp)
  -> in a mission: GamePlay (src/game/main.cpp ~566)
       handles pause, Esc, Help, volume, music, fast motion itself (not part of the simulation)
       passes everything else to the InputSource (src/game/input-source.cpp)
  -> RealInputSource queues each event for the next major tick (every 3rd tick)
  -> at that tick PlayerShip (src/game/player-ship.cpp) receives it, turns it into
     PlayerEvents and key bits, and PlayerShip::update() applies them to the flagship
```

- **The simulation only changes through that queue.** Keys, clicks and (on macOS) gamepad
  events all reach `PlayerShip` at major ticks, so the game stays deterministic. Anything new
  that changes game state (an order, a selection, a zoom) has to go the same way.
- **Not registered with GLFW:** the scroll wheel (`glfwSetScrollCallback` is never called) and
  gamepads. **The gamepad is only read by the macOS driver** (`src/mac/video-driver.cpp`); the
  GLFW driver used on Windows never polls one, so the gamepad bindings in `docs/current-state.md`
  §1.4 don't work on Windows today.
- Mouse buttons other than left (0) and right (1) are ignored in a mission
  (`PlayerShip::mouse_down`).

## 2. Every input today

Context: **battle** = in a mission; **menu** = interface screens; **any** = everywhere.
"Prefs" means the binding is one of the 44 rebindable actions (`KeyNum`,
`include/config/keys.hpp` ~214), stored in `config.pn` and edited in Options › Key Controls.

### 2.1 Keyboard, battle (rebindable)

| Input (default) | Action | Context | Read in / decided in |
|---|---|---|---|
| Keypad 8 / 5 / 4 / 6 | Accelerate / decelerate / turn CCW / turn CW (held) | battle | `PlayerShip::key_down/up` → key bits → flagship `keysDown`, `player-ship.cpp` ~328–460 |
| Left Alt / Left Win | Fire weapon 1 / 2 (held) | battle | same |
| Space | Fire or activate special (held) | battle | same |
| Tab | Warp (held); with Target held: autopilot to target | battle | same; `TogglePlayerAutoPilot` ~1102 |
| Keypad Clear | Select friendly (cycle control in facing direction); with Target: target a friendly | battle | `select_object`/`pick_object` ~200–296 |
| Keypad = | Select hostile (always targets) | battle | same |
| Keypad / | Select base; with Target: target any base | battle | same |
| Shift | Target modifier; held alone ≥ 0.75 s then released: target own ship | battle | `use_target_key()`, `PlayerShip::key_up` |
| Ctrl | Order to go: control object to the target | battle | `PlayerEventType::ORDER` → `PlayerShipGiveCommand` ~1119 → `OverrideObjectDestination` (`admiral.cpp` ~437) |
| Keypad + / − | Zoom one step in / out | battle | `zoom_in`/`zoom_out`, `player-ship.cpp` ~175–190 |
| Up / Down / Right / Left | Minicomputer previous / next / accept / cancel | battle | `minicomputer.cpp` |
| F8 | Transfer control | battle | `transfer_control` (`minicomputer.cpp` ~815) |
| F9 / F10 / F11 / F12 | Zoom to 1:1 / 1:2 / 1:4 / 1:16 (again: back to previous) | battle | `zoom_shortcut`, `player-ship.cpp` ~160 |
| Help (no PC key) | Zoom to nearest hostile | battle | same |
| Home | Zoom to nearest object | battle | same |
| Page Up | Zoom to show all | battle | same |
| Backspace | Next message page / clear | battle | `Messages`, via PlayerEvent |
| F1 | Help screen (pauses) | battle | `GamePlay::key_down`, `main.cpp` ~593 |
| F2 | Lower volume: **does nothing** (`set_volume(volume())`) | battle | `main.cpp` ~602 |
| F3 | Raise volume | battle | same |
| F4 | Toggle in-game music | battle | same |
| F5 | Expert net settings: **no effect** (no network play) | battle | explicitly ignored, `player-ship.cpp` ~394 |
| F6 | Fast motion, 12× while held | battle | `GamePlay::key_down/up`; `fire_timer` ~440 |
| 1 … 9, 0 | Hotkeys: tap = select (or target with Shift); hold ≥ 0.85 s = assign | battle | `PlayerShip::key_down/up`, `HOTKEY_SET` |

**Unreachable defaults on a PC keyboard:** GLFW has no key for Keypad Clear or Help, and most PC
keypads have no `=` (`glfw_key_to_usb`, `video-driver.cpp` ~60–160; Num Lock maps to nothing).
So **Select friendly, Select hostile and Zoom to nearest hostile have no working default key**
on Windows until rebound.

### 2.2 Keyboard, fixed

| Key | Action | Context | Source |
|---|---|---|---|
| Esc | Play Again / pause menu (not in replays) | battle | `GamePlay::key_down`, `main.cpp` ~574 |
| Caps Lock | Pause; released: resume | battle | `main.cpp` ~568; `PauseScreen` |
| Return | One-line text entry (cheats, naming); Esc cancels | battle | `PlayerShip::MessageText` |
| Alt+Enter | Fullscreen / windowed | any | `GLFWVideoDriver::key`, `video-driver.cpp` ~214 |
| 1 … 0 | Show the n-th object's data sheet | briefing | `src/ui/screens/briefing.cpp` |
| Any key or click | Skip intro, fades, scrolling text | intro | `transitions.cpp`, `scroll-text.cpp` |
| Esc / Return / Caps Lock | Can't be bound; cancel binding capture | Key Controls | `KeyControlScreen::key_down`, `options.cpp` ~409 |

**Menu shortcuts** come from the interface data files (`data/interfaces/*.pn`, `key:` fields)
and can't be rebound:

| Screen | Keys |
|---|---|
| Main menu | S start, N network (disabled), O options, Q quit, A about, D demo, R replay intro |
| Select level | Return begin, Esc cancel, Left / Right previous / next |
| Briefing | Return done, Left / Right previous / next |
| Options (sound, video, keys) | Return done, Esc cancel, S sound, V video, K key controls |
| Play Again | Return start over, Q quit to menu, Esc resume, S skip tutorial |
| Help | Esc done |

### 2.3 Mouse

| Input | Action | Context | Source |
|---|---|---|---|
| Move | Shows the crosshair cursor; hides after 1 s idle. While shown, keyboard select/target keys are ignored | battle | `src/game/cursor.cpp`; `PlayerShip::update` |
| Left click on a friendly | Select it as **control** (within ±16 px) | battle | `PlayerShipHandleClick`, `player-ship.cpp` ~1025 |
| Right click on any object, or Shift + left click | Make it the **target** | battle | same |
| Click / double-click | Highlight / execute a minicomputer line | battle | `MiniComputerHandleClick/DoubleClick` |
| Middle button, extra buttons | Ignored | battle | `PlayerShip::mouse_down` |
| Scroll wheel | **Not read anywhere** | any | no `glfwSetScrollCallback` |
| Click | Buttons, checkboxes, tabs | menu | `screen.cpp`, `widget.cpp` |

There is **no box select, no click-to-order, no camera movement by mouse.** Orders always need
the keyboard (Ctrl) or the minicomputer.

### 2.4 Gamepad (macOS only today)

Hard-wired in `PlayerShip::gamepad_*` (`player-ship.cpp` ~519–692) and
`GamePlay::gamepad_button_down`: left stick steers (turns toward the stick direction), A thrust,
B brake, RT fire 1 and 2, LT special, LSB warp, X / Y zoom out / in, Back next message, D-pad
minicomputer, Start Play Again menu. LB / RB turn A, B, X, Y, LSB into target / select / order /
autopilot / transfer commands aimed with the stick. Not rebindable, not stored in prefs.

## 3. Bindings today and what full rebinding needs

**Storage:** `Preferences::keys[44]` (`include/config/preferences.hpp`), one USB HID key code per
action, defaults in `Preferences::Preferences()` (`src/config/preferences.cpp` ~43–93), saved
in `config.pn` under `keys:` by action name (`src/config/file-prefs-driver.cpp`). One key per
action, keyboard only. The Options › Key Controls screen edits them: five tabs (Ship, Command,
Shortcuts, Utility, Hot Keys), conflicts flash and block Done until fixed, no reset to defaults.

**Hard-wired, and why:**
- Esc, Caps Lock, Return in battle: the pause, Play Again and text-entry screens are separate
  cards pushed by `GamePlay`; they predate the prefs and are also the keys the binding screen
  uses to cancel.
- Menu shortcuts: part of the interface data (`data/interfaces/*.pn`), which belongs to the
  separately licensed data repository.
- Mouse buttons: left = select, right = target, fixed in `PlayerShip::mouse_down`.
- Gamepad: a fixed switch in `player-ship.cpp`, with the bumper modes as a small state machine.
- Briefing 1–0 and Alt+Enter: handled directly by their screens / the video driver.

**What every action rebindable would take:**
1. A binding type that can hold a key, a mouse button, the wheel (up / down) or a gamepad button,
   with two bindings per action (primary and secondary), replacing `Key keys[44]`.
2. New action numbers appended after the existing 44 (pause, Play Again, text entry, the new
   Modern actions). **The existing numbers 0–43 must not change:** replay files store action
   numbers and map them back to keys through the prefs on playback
   (`ReplayInputSource`, `input-source.cpp`).
3. A versioned `config.pn` format that still reads today's `keys:` map.
4. Presets (Classic, Modern) as complete binding sets, with "reset to preset".
5. A new rebinding screen built in code (like the UI-scale and Graphics choices in Video
   options), since the current Key Controls layout is a fixed data file with a slot per action
   and five tabs; it needs scrolling, capture of keys / mouse / gamepad, per-context conflict
   rules (battle vs menu) and a preset selector.
6. Menu shortcuts can stay data-driven; they don't clash with battle bindings because menus and
   battle are different cards.

## 4. Mouse, camera and zoom today

- **Selection:** one **control** object and one **target** object per side (`Admiral`). A click
  picks the nearest suitable sprite within ±16 px (`GetSpritePointSelectObject`). There is no
  multi-select: everything (orders, the minicomputer's Build and Special Orders, the CONTROL line)
  works on the single control object.
- **Orders:** "go to target" (Ctrl) sets the control object's destination to the target object.
  Hold Position and Go To My Position use a **point** destination
  (`SetObjectLocationDestination`), so the game can already send a ship to a location, but no
  input does that for an arbitrary point.
- **Camera:** always centred on your flagship (`scaled_screen.bounds`, `motion.cpp` ~421). No
  panning, no edge or drag scrolling.
- **Zoom:** eight fixed levels in `g.zoom` (2:1, 1:1, 1:2, 1:4, 1:16, nearest foe, nearest
  object, all; `include/data/enums.hpp`). `g.zoom` is **game state**: level conditions test it
  (`ZoomCondition`, used by tutorials) and it changes which object counts as "closest"
  (`motion.cpp` ~541). The drawn scale eases toward the chosen level (`instruments.cpp` ~268).
- **Wheel zoom would need:** a scroll callback in the GLFW driver, a wheel event type through
  `Card`/`GamePlay`, and mapping wheel steps to the existing zoom-in / zoom-out actions through
  the input queue (so `ZoomCondition` and determinism still hold). Stepping should cover 2:1 to
  1:16 (and perhaps "all"); today's zoom-out key also walks through foe and object, which feels
  odd on a wheel. Continuous zoom would need new zoom levels in level data: not recommended.
- **Wide windows:** the play area is the window minus the side panels (128 + 32 logical px),
  centred on the flagship, and the auto zooms (foe, object, all) fit to its **height** only. At
  5120×1440 and UI 2× it is 2400×720 logical, 3.3:1, against the original's 480×480: you see far
  more to the sides than the scenarios were designed for. The simulation itself doesn't depend
  on the view (only a distance fallback uses the screen centre when you have no ship). This is
  the existing open question on capping the field of view.

## 5. Presets

**Classic** = today's defaults, kept as they are, except for the fixes that make them reachable
on a PC (see stage 1): Select friendly, Select hostile and Zoom to nearest hostile get PC keys
(proposal: Keypad `.`, Keypad `*`, End), F2 lowers the volume.

**Modern** differs as follows (⚠ = awkward with the game's design):

| Action | Classic | Modern | Notes |
|---|---|---|---|
| Thrust / brake / turn | Keypad 8 5 4 6 | W S A D | rebinding only |
| Fire 1 / 2 / special | L-Alt, L-Win, Space | Left mouse, right mouse, Space | needs mouse bindings; ⚠ clashes with click-to-select and right-click target, so Modern must choose: fire on mouse buttons only while a modifier is held, or fire on keys (proposal: keys J / K, mouse stays for selection and orders) |
| Aim | turn keys | **new:** turn the flagship toward the cursor while a key is held | reuses the gamepad "turn toward stick direction" logic, through the input queue |
| Select | Keypad Clear / = / /, left click | left click selects; Tab-like cycling on Q / E | |
| Target | Shift modifier, right click | right click on an object | |
| Order | Ctrl | **new:** right click on an object = target it and order the control object to go there; right click on empty space = **new** "move to point" | point destinations exist (`SetObjectLocationDestination`); new player event |
| Zoom | Keypad + / −, F9–F12 | **mouse wheel**, plus F9–F12 | new wheel input |
| Camera | follows the flagship | **new:** edge or middle-drag scrolling, **new** "recentre" key | ⚠ the original never moves the camera off the flagship; view-only, but the radar, auto zooms and click picking assume the flagship at the centre |
| Control groups | 10 hotkeys, one object each | **new:** Ctrl+number assigns several ships, number selects them | ⚠ the game has one control object; a group would need orders issued to every member, and the minicomputer shows one CONTROL |
| Box select, select all of type | none | **new** | ⚠ same reason: no multi-select model |
| Camera jump | none | **new:** jump to control / target | view-only |
| Pause, menu | Caps Lock, Esc | P (or Pause), Esc | needs pause to become an action |

Gamepad: both presets share the macOS mapping once Windows reads gamepads; a Modern pad layout
can follow later.

## 6. Staged plan

| Stage | What | Done when | Risk |
|---|---|---|---|
| 1 | **PC defaults and small fixes:** reachable keys for Select friendly / hostile and Zoom to nearest hostile; F2 lowers the volume; reset-to-defaults button in Key Controls | Every default action works on a standard PC keyboard; old `config.pn` files keep their own bindings; replay tests pass | Low. Changing a default changes nothing for players who rebound it |
| 2 | **Mouse wheel zoom:** scroll callback → zoom in / out actions through the input queue; steps 2:1 … 1:16 | Wheel zooms in game at UI 1×–4×; tutorial `ZoomCondition`s still fire; `g.key_mask` can disable it; replay tests pass | Low–medium: zoom is game state, so it must go through the queue |
| 3 | **Gamepad on Windows:** poll GLFW gamepads, emit the existing gamepad events | An Xbox-style pad flies, fights and uses the minicomputer as on macOS | Low; the game logic already exists |
| 4 | **Binding model and presets:** key / mouse / wheel / gamepad bindings, two per action, actions appended after 43, versioned config, Classic and Modern presets, preset choice in Options | Old configs load unchanged; switching presets changes bindings; replay tests pass (action numbers 0–43 unchanged) | Medium: config format and replay compatibility |
| 5 | **Rebinding screen** in code: scrolling action list, capture any input, conflict rules per context, reset to preset | Every rebindable action can be rebound with keyboard, mouse or gamepad and survives a restart | Medium: largest UI piece; may wait for the M3 look |
| 6 | **Modern mouse actions:** right-click order, move to point, turn toward cursor, WASD defaults | ch01–ch04 and the tutorials playable with Modern alone; new events respect `g.key_mask`; replay tests pass | Medium: new player events in the simulation path |
| 7 | **Camera:** edge / drag scrolling, recentre, camera jumps; decide the field-of-view cap | Panning never changes game state (replays identical); recentre works; FOV decision recorded | Medium: drawing, radar and click picking assume a flagship-centred view |
| 8 | **Optional, needs a design decision:** control groups, box select, select all of type | Decided first whether multi-select fits "faithful first"; then groups give each member the order | High: changes the single-control model the minicomputer and orders are built on |

**Cross-cutting risks**
- **Determinism and replays:** anything that changes game state (orders, selection, zoom, aim)
  must enter through `InputSource` at a major tick. Replays store action numbers 0–43 and map them
  through the current bindings, so those numbers can't change. Replay **recording** is currently
  switched off (`_replay_builder.init` is commented out in `main.cpp` ~162), records only
  keyboard actions, and its key-up check is inverted (`ReplayBuilder::key_up` returns when the
  file *is* open). If recording comes back, it should record actions, not devices.
- **Tutorials:** levels disable keys and the mouse through `g.key_mask` (for example
  `kMouseMask`, the minicomputer masks). New actions need masks, or a tutorial step can be
  skipped by the new input.
- **Network play:** not implemented (the main menu's network entry is disabled; F5 does
  nothing), so no network code is at risk now. Events are already queued per admiral, which a
  later network mode would rely on.
- **Data files:** the Key Controls layout and menu shortcuts live in the separately licensed
  `data/interfaces/`; new UI should be built in code, as Video options already is.

**Correction for `docs/current-state.md` §1.4 (not changed here):** the gamepad mapping is
real but only the macOS driver reads gamepads; on Windows no gamepad input arrives.

## 7. Outcome of stages 1–3 (2026-10-20)

Each stage is its own commit with unit tests; a clean rebuild passes all 26 tests (16 replays
unchanged: no action numbers moved).

| Stage | Commit | What changed | Tested |
|---|---|---|---|
| 1 | `4d591f5e` | Select friendly, Select hostile and Zoom to nearest hostile default to Keypad `.`, Keypad `*` and End. A saved config still holding one of Ares' dead defaults (Keypad Clear, Keypad =, Help) is moved to the new key on load unless another action uses it; other bindings are kept. F2 lowers the volume (it did nothing); F3 stops at the maximum. Key Controls has a **Reset to Defaults** button (Cancel undoes it). | `prefs-test` (defaults reachable and distinct, the move, an old config file). In game at UI 2×, starting from an old-format config with the three dead defaults and Fire 1 on Q: the saved config had the new keys and Q kept; F2 twice took the volume 7 → 5; Keypad `*` targeted the hostile, Keypad `.` selected a friendly, End switched to the nearest-hostile zoom; Reset to Defaults put Fire 1 back to Option. |
| 2 | `d7ddff19` | The GLFW driver reads the scroll wheel (fractions add up to notches) and sends a `MouseWheelEvent`; `GamePlay` queues it in the input source like a key, so it reaches `PlayerShip` at a major tick as the same `ZOOM_IN`/`ZOOM_OUT` player event. Up steps toward 2:1, down to 1:16 and stops; from an automatic zoom, up goes to 1:16. Levels' zoom masks apply to it; the mouse mask doesn't (tutorial 1 enables zooming while the mouse is off). | `controls-test` (step rule). In game, 5120-wide window, UI 1×, 2×, 3×, 4×: 4 notches up reach 2:1, 6 down return to 1:16. A level `zoom` condition fired on the wheel (the probe level's 2:1 condition reset the zoom to 1:4). With zoom-in disabled by the level, the wheel did nothing. Replays neither record nor react to it. |
| 3 | `3cb6bd5c` | The GLFW driver polls the first standard-mapping gamepad once a frame; `GamepadPoller` sends the gamepad events the game already handles (buttons, LT/RT with hysteresis, left stick as 32 directions with a dead zone, only on change; a disconnect releases everything). Gamepad input switches on-screen labels to gamepad names. | `controls-test`: button mapping, triggers, dead zone and steps, disconnect, and each stick direction steering the flagship that way through the game's rotation table (it caught a mirrored x before it reached the game). **No real controller was connected, so it hasn't been tried with one.** |

`docs/current-state.md` §1.2–1.4 describe the new behaviour; §1.4 now says the gamepad was read only
on macOS until stage 3.

## 8. Outcome of stages 4–5 (2026-10-20)

| Stage | Commit | What changed | Tested |
|---|---|---|---|
| 4 | `ae9b6f18` | **Binding model.** Each action has a primary and a secondary binding (key, middle or side mouse button, wheel direction) and a gamepad button. Two actions are added after the original 44 (Attack move 44, Patrol 45; they do nothing yet). Every device goes through one path: `PlayerShip` presses the actions bound to an input, counting held inputs per action; `GamePlay` does the same for help, volume, music and fast motion. Input still goes through the per-tick queue. **Presets** Classic (default; today's controls, the wheel and the Ares gamepad mapping) and Modern (WASD; attack move G, patrol P) are data tables; Key Controls got a preset choice. **Config version 2** (`controls` section), still writing version 1's `keys`; a version 1 file loads as Classic with its keys. **Replays** play back as action numbers, independent of bindings. Reserved: Esc, Return, Caps Lock, left and right mouse, the pad's bumpers and Start. | `prefs-test`: 10 new tests (presets, Classic = today, clashes, slots, reserved inputs, config values, version 2 round trip, version 1 load, invalid bindings ignored). All 26 tests pass; the 16 replays are unchanged. |
| 5 | (this commit) | **Rebinding screen**, Options › Key Controls, built in code on the old screen's frame: six pages, the preset choice with "(CHANGED)", Primary / Secondary / Gamepad cells; click a cell and press an input; Esc cancels; Clear; clashes flash and block Done; Reset to Defaults restores the preset. | `prefs-test`: every action on exactly one page, each page fits (11 rows). In game (windowed, test configs), UI 1×, 2×, 3×, 4×: the screen draws the same at every scale. At UI 2×, starting from a version 1 config: Preset Modern filled in WASD; the Modern page shows G and P; Q on Accelerate clashed with Select friendly (gold cells, message, Done and Options disabled), W fixed it; wheel up on Decelerate clashed with Zoom in, and Clear removed it; the middle button on Lower volume showed as MOUSE 3; Done saved a version 2 config (preset modern, `{mouse: 2}`). In the level, two middle-button presses took the saved volume from 7 to 5, and Modern's "=" zoomed in. |

**Not tested:** gamepad rebinding with a real pad (none is connected); the gamepad cells and the pad
path are covered by the binding tests and the stage 3 poller tests only.

Stages 6–8 are untouched; Modern behaviour (selection, orders, attack-move, patrol) is
`docs/modern-controls.md`.
