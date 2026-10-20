# Star Bound: Design Document (v0.1)

Working title. Based on Ares (Nathan Lamont) via the Antares open-source port.

## 1. Vision

A faithful, modernised version of Ares for Windows PC players. The gameplay, scenarios and story stay as they are. The controls, interface and graphics are brought up to modern PC standards.

## 2. Principles

- **Faithful first.** Rules, units, weapons, orders, AI and scenarios behave as in the original.
- **Always playable.** Every change is small, tested and committed. The game must work after each step.
- **Fallbacks.** Original art and the classic control scheme stay available until replacements are proven.
- **Written down.** Decisions go in this file so every session (human or Claude Code) starts from the same facts.

## 3. What stays unchanged

Core rules, unit and weapon behaviour, fleet orders, AI, scenarios, story text. (To be confirmed against `docs/current-state.md`.)

## 4. Controls

- Two built-in presets:
  - **Classic:** the original scheme, unchanged.
  - **Modern:** C&C-style fleet control with the mouse: select several ships (box, shift-click, Ctrl+number groups), right-click to move or attack, patrol and attack-move commands; flight and firing stay on keys. Mouse-wheel zoom works in both presets.
- A rebinding screen so every action can be reassigned to keyboard keys or mouse buttons.
- Bindings are saved in the player's config and survive updates.
- Gamepad: the game logic and a fixed mapping exist (see `docs/current-state.md` §1.4), but **only the macOS driver reads gamepads; on Windows no gamepad input arrives** (finding, 2026-10-19). Both presets and the rebinding screen must cover gamepad as well as keyboard and mouse.
- Mouse-wheel zoom needs new input handling: the game currently ignores the scroll wheel. M2 includes adding it.
- Exact action list to be taken from `docs/current-state.md`.
- **Investigation (2026-10-19):** `docs/controls-investigation.md` maps every input, the binding storage and the replay and tutorial constraints, and proposes the presets and an 8-stage plan (smallest first: PC-reachable defaults, wheel zoom, gamepad on Windows, then the binding model, rebinding screen, Modern mouse orders and camera). Constraints found:
  - anything that changes game state (orders, selection, zoom) must go through the per-tick input queue to stay deterministic;
  - replay files store action numbers 0–43, so they can't be renumbered;
  - tutorials disable inputs through `g.key_mask`, so new actions need masks.
- **Goal (2026-10-20): a Classic / Modern control toggle in Options, Classic by default.** Modern adds:
  - multi-ship selection: drag a box, shift-click to add, Ctrl+number control groups;
  - right-click orders;
  - a patrol command;
  - an attack-move command.
- **Decisions (2026-10-20, Modern controls plan approved):**
  - Modern flight stays on WASD.
  - Attack-move defaults to Ctrl+right-click plus a rebindable key (G in the Modern preset).
  - The on-screen pointer is enough for a double-tapped group number until the camera work.
- **Decision (2026-10-20): the binding model (M2 stage 4).**
  - Each action has a primary and a secondary binding (key, extra mouse button or wheel) and a
    gamepad button. Classic needs three inputs on zoom (keypad, wheel, pad), and a separate pad
    slot keeps a pad binding from displacing a keyboard one.
  - New actions go after the original 44, whose numbers never change.
  - Classic and Modern are presets kept as data tables, chosen in Options; Classic is the default
    and is today's controls.
  - The config is version 2 and still reads and writes version 1's `keys`.
  - Replays play back as action numbers, independent of bindings.
  - Inputs the game keeps for itself can't be bound: Esc, Return, Caps Lock; left and right mouse;
    the pad's bumpers and Start.
  - See `docs/current-state.md` §1.1.
- **Done (2026-10-21): Controller detection off the startup path** (`docs/startup-timing.md`). A start sometimes took about 30 s, likely in GLFW's DirectInput enumeration inside `glfwInit()`. Windows now builds GLFW with an altered joystick file in our tree (the submodule is unchanged): XInput is detected at startup, and DirectInput is scanned on a worker thread after the main menu shows, with the found pads opened on the main thread. New `--timing` marks cover sound, `glfwInit`, the window and the scan.
- **Done (2026-10-21): Modern attack-move and patrol** (`docs/current-state.md` §2.4d). Built from the engine's existing point order, with no new order type: attack-move leaves the 15 s engage clock at 0 (the Classic point order), and two-point patrol keeps per-ship records in the game state that a per-tick step turns at each end, dropping a record when anything else (a script or another order) changes the ship's destination (`docs/modern-controls.md` §5b).
- **Done (2026-10-20): Modern control groups** (`docs/current-state.md` §2.4c). In the Modern mode the number keys are control groups (Ctrl+number saves, number recalls, Shift+number adds, double tap shows a pointer) and Classic keeps its hotkeys. The Order key fires on release unless a number was pressed while it was held, which resolves the Ctrl clash in Modern: WASD (the approved plan, `docs/modern-controls.md` §5.1).
- **Done (2026-10-20): Modern right-click orders for one selected ship** (`docs/modern-controls.md` step B, `docs/current-state.md` §2.4a).
  - The Modern mode is on with a Modern preset, and off in replays, the demo and the tutorials. This
    settles the open question on tutorials: they always run Classic.
  - Left-click selects. A right-click attacks an enemy, follows a friendly ship, guards a friendly
    base, or moves to empty space.
  - The flagship is never ordered.
  - Orders are the existing destinations, so scripted orders still override them.
- **Decision (2026-10-20): three presets.**
  - Classic is unchanged.
  - **Modern: Ares** (Ares-style) is the default Modern layout: the arrows steer, as Ares'
    keypad 8-4-5-6 did; fire and action keys are under the left hand; attack-move is A, then click
    (plus Ctrl+right-click). Ctrl, Alt and the Windows keys are left unbound.
  - **Modern: WASD** is the alternative, with Patrol moved from P to T.
  - Labels: "CLASSIC", "MODERN: ARES", "MODERN: WASD". A config saved with stage 4's "modern"
    preset loads as Modern: WASD.
  - Tradeoff accepted: in Ares-style the right hand can't steer and use the mouse at once. Nothing
    is added to mitigate it now; ships coast, autopilot flies the flagship, and WASD is the choice
    for steering while using the mouse.
- **Investigation (2026-10-20, revised the same day):** `docs/modern-controls.md` covers how orders and ship behaviour work, the detailed behaviour of control groups, drag selection with context-sensitive right-click, attack-move and patrol, and the proposed build order (toggle, right-click orders, attack-move, multi-select, groups, two-point patrol, then multi-point patrol routes). Findings:
  - Ctrl orders to an object fly straight there; a ship only fights after being hit or on arrival.
  - Point orders (Hold Position, Go To My Position) engage enemies met on the way, giving up after 15 s of fighting a target that doesn't fire back.
  - So move reuses existing behaviour, attack-move is that point order without the 15 s limit, and patrol is new.
  - Nothing in `data/` needs to change, provided the admiral keeps a single primary control object (tutorials test it) and scripted orders cancel Modern orders.
  - Replays, the demo and tutorials should always run Classic.
  - Right-click on a friendly ship or base maps to the existing escort and guard behaviours.
  - The camera is locked to the flagship, so a double-tapped group number can only point to the group (an off-screen label pointer and radar highlight), not jump to it.
  - A group member that dies or is captured drops out. A group whose ships are all gone selects nothing and plays the warning sound.
  - Newly built ships head for the player's current target, as they always have.
  - Antares doesn't enforce the tutorials' masks for the select, target, order and minicomputer-navigation keys (an existing gap).
  - Proposed for review, not yet decided:
    - tutorials always run Classic;
    - attack-move is A then left-click, with Ctrl+right-click as a second gesture;
    - patrol is P then left-click, with Shift+click adding waypoints (up to 8, built after two-point patrol);
    - formations, queued move orders, and mouse control of the flagship are left out.

## 5. User interface

- Modern HUD and menus with a clean sci-fi look.
- Resolution-independent layout: scales correctly at any size and aspect ratio, including ultrawide.
- Menus fully usable with the mouse; keyboard shortcuts kept.
- First step: fix the widescreen scaling problem, since the redesign depends on it.
- **Decision (2026-10-18): UI scaling uses a global integer scale** (option A of the scaling review). The whole game is drawn at a logical resolution of window ÷ N and magnified N× with crisp nearest-neighbour pixels, reusing the existing HiDPI mechanism. Details:
  - N is automatic: ⌊window height ÷ 540⌋, minimum 1 (1080p and 1440p → 2×, 4K → 4×). It is capped so the logical screen never drops below 640 × 480, the size the menus and HUD are laid out for.
  - Video options has an override: Auto, 1×, 2×, 3×, 4× (still capped as above). It applies immediately; Cancel reverts it.
  - Mouse coordinates are converted exactly. When the window isn't a multiple of N, the leftover pixels form a thin black border.
  - This also magnifies the battle view, so it shows less of the battlefield than before at large window sizes. That is closer to the original field of view. Wider windows still show more horizontally; capping that is a separate open question.
  - A full resolution-independent redesign (anchored layouts, new art) remains M3.

## 6. Graphics

- Ships are modelled in Blender and rendered to sprites by a script, so the game keeps its top-down view and gameplay.
- Pipeline: model, consistent lighting and camera, render a full set of rotation frames, export in the format the game already loads.
- Ships are replaced one type at a time. Anything not yet replaced uses the original sprite.
- After ships: planets, stations, weapon effects, explosions and thruster effects.
- High-resolution sprites (finding, 2026-10-18): the game uses each sprite frame's size and centre from the `.pn` table for **collision bounds** (`calc_bounds`, `src/game/motion.cpp`), not just for drawing. New art must therefore keep the original 1× frame rectangles and centres, and supply extra detail as `@2x`/`@4x` images used only as textures. Pictures and fonts already load `@2x` files automatically. Sprites now do too (see the decision below and `docs/current-state.md` §4.3).
- **Decisions (2026-10-18): sprite render pipeline.**
  - **Keep the original frame geometry.** New sprites use each original frame's rectangle and pivot unchanged, and the `.pn` tables are never rewritten. Collisions, weapon mounts and replays stay identical. New silhouettes must fit the old frames; the render script's fit check reports any overflow.
  - **Prove the pipeline with a placeholder.** The test ship is the Human cruiser (`uns/cruiser`, 24 frames). The script builds a placeholder from basic shapes (capsule hull, three engine pods). A real model replaces it later.
  - **Test through the scenario override folder.** Rendered 1× sheets go into `Documents/Antares/Scenarios/<factory scenario>/sprites/…`, which the game reads before `data/`. Nothing in `data/` or the game code changes for a test, and a tool removes the files afterwards. Where finished art will live permanently is still open.
  - **Render 1×, 2× and 4×.** The game now loads all three (next decision).
- **Decision (2026-10-18): sprites load `@2x`/`@4x` sheets when present.** This reuses the HiDPI draw scale. For each sprite the game picks the smallest sheet at least as detailed as the screen, then larger, then smaller: UI 1× uses 1×; 2× uses @2x; 3× and 4× use @4x. If a sheet is missing, or isn't an exact multiple of the 1× size, it falls back to the next one and ultimately to the original. Frame rectangles and pivots stay in 1× units, so collisions, weapon mounts and replays are unchanged. Tested with the placeholder cruiser at UI 1×–4×, including the fallbacks.
  - **Amended 2026-10-18:** the sheet is chosen from the effective draw scale, UI scale × the object's own sprite scale. So capital ships, drawn at 2×, use @2x at UI 1× and @4x from UI 2×.
  - `ASSETS.md` stays unchanged until a real model replaces an original sprite.
- **Finding: the original sprites use a three-quarter view.** They were rendered with a camera tilted about 44° from vertical, viewing from the bottom of the screen, which shortens ships' length on screen. Each ship's pivot also sits above the hull. New renders have to match both, or ships won't line up with their pivots. The angle was fitted for the Human cruiser; other ships give noisier estimates (8°–52°), so the tilt is a per-sprite setting (`--tilt`). Tools: `tools/sprite-render/`, described in `docs/current-state.md` §4.3.

- **Decision (2026-10-19): upscale the originals instead of remodelling them.** Ships get @2x/@4x sheets upscaled from their own pixels:
  - Real-ESRGAN `realesr-animevideov3` for the image where it looks better, tolerant Scale2x otherwise;
  - tolerant Scale2x for every overlay, so team colour stays faithful.

  Per-sprite choices are in `docs/upscale-decisions.md` (16 still marked unsure for review; four were switched to tolerant Scale2x after the in-game check). `ASSETS.md` records the upscales as adaptations (CC BY-NC-SA); `CREDITS.md` credits Real-ESRGAN.
- **Decision (2026-10-19): Enhanced applies at every UI scale.** At UI 1x the AI-image sprites use a 1x image shrunk from their AI @2x sheet. Since 2026-10-19 every rotating ship is enhanced at every scale: all but the Audemedon assault transport and Cantharan gateship use the AI image (61), even where the review found it clearly worse (listed for review in `docs/upscale-decisions.md`); the 12 tiny or non-ship sprites stay original.
- **Decision (2026-10-19): players choose Original or Enhanced graphics.** Video options has a Graphics choice: Enhanced (default) uses `art/`, Original shows only the original art, as the fallback principle asks. It is saved in `config.pn` and takes effect from the next level; `--art DIR` and `--no-art` override it.
- **Decision (2026-10-19): acceptance gate for remastered ships.** A remaster goes into `art/` only if, in game at UI 2×, it looks better than the original and keeps its features; otherwise the ship stays original. Appearance (per-region colour and brightness spread, SSIM) is measured alongside the outline.
  - The Ishiman ships stay original. Their originals were rendered nearly overhead, so fitted shapes come out plainer than the hand-detailed originals (`docs/fleet-plan.md`, batch 1 outcome).
- **Finding (2026-10-18): remaster ships from the original frames, not by eye.** The three hand-built cruiser variants drifted from the original design. A remaster now:
  1. measures the shape by carving it out of the 24 frames;
  2. fits a clean parametric model to the frames;
  3. renders that model, so the silhouette is checked against a metric;
  4. takes base colours from the original's colour regions.
  - Targets: IoU mean ≥ 0.88, worst ≥ 0.85, outline mean ≤ 0.5 px, worst p95 ≤ 1.5 px.
  - The Human cruiser trial reaches 0.91 / 0.88 / 0.38 px / 1.41 px. The carving ceiling is 0.97 / 0.94 / 0.12 px; the old variants scored 0.79 / 0.73 / 0.92 px.
  - The frames only show one elevation, so section heights, the underside, panel lines and materials are invented.
  - Approved 2026-10-18 and tested in game at UI 1×, 2× and 4× (`docs/current-state.md` §4.3). Details: `docs/current-state.md` §4.3.1.
  - Two more Human ships done the same way (2026-10-18):
    - fighter: IoU 0.90 / 0.87, outline 0.29 px;
    - carrier: IoU 0.94 / 0.92, outline 0.46 px; its p95 is 2.0 px, from frame 0's 1 px antenna (miss accepted 2026-10-18).
  - They use two reusable shape kits: twin-pod and box-kit. Per ship, the hand-written part is a
    30–35 line shape module (layout, starting values, measured look); camera finding, carving,
    fitting, offset, colour calibration and the comparison sheet are automatic.
  - Ships that fit an existing kit took about 7 minutes of agent time; the carrier, which needed
    the new box kit, took about 12. Compute is 2–4 minutes per ship.

## 7. Out of scope for now

New factions or missions, multiplayer, moving to Unity or Unreal, real 3D gameplay. Revisit after the milestones below.

## 8. Licensing and naming

- Antares code is open source, so Star Bound's source must stay open.
- Original Ares art, sound and text are licensed non-commercial and share-alike. Fine for a free game. To ever sell it, every original asset must be replaced.
- Keep `ASSETS.md` listing each asset and whether it is original, replaced, or new.
- "Star Bound" is a working title only. A commercial game called Starbound already exists, so choose a distinct name before release.

## 9. Milestones

| # | Milestone | Done when |
|---|-----------|-----------|
| M0 | Stage 1: Antares runs on Windows | Done: builds, plays, packaged, in git |
| M1 | UI scales with the window | Menus and HUD scale crisply with the window (automatic integer scale plus Video options override), mouse input stays exact, tested at 1080p, 1440p, ultrawide and small windows, windowed and fullscreen |
| M2 | Controls | Classic and Modern presets plus a rebinding screen |
| M3 | UI redesign | New HUD and menus replace the old ones |
| M4 | Blender pipeline | One ship rendered from a script and working in game (in progress: script done; game loads @2x/@4x sprite sheets; placeholder Human cruiser rendered and tested in game at UI 1×–4× via the override folder; Human cruiser remaster fitted to the original frames and tested in game at UI 1×, 2×, 4× beside original ships; Human fighter and carrier remastered with shared shape kits, approved, and all three tested in game at UI 1×, 2×, 4×; full-fleet plan next) |
| M5 | All ships replaced | Every ship type uses new sprites (2026-10-19: 63 ship sprites have upscaled @2x/@4x sheets in `art/` per `docs/upscale-decisions.md`, 3 Human ships are 3D remasters, 12 sprites stay original; tested in game at UI 1x-4x) |
| M6 | Effects | New explosions, weapons, thrusters, lighting |

## 10. Open questions

- Which parts of the UI need a full redesign and which only need scaling?
- ~~How many ship types and frames per ship does the game load?~~ Answered 2026-10-18: 78 rotating sprites are in use, by 106 objects (corrected from "82 objects" by `fleet_inventory.py`); all have 24 frames (15° steps) except the Bazidan battleship (36). 70 are ships, 8 are missiles, effects or figures. Frames range from 12 × 11 to 108 × 78 px (see `docs/current-state.md` §4.3).
- ~~Where should finished replacement art live?~~ Decided 2026-10-18: an `art/` folder at the repository root, read after the factory scenario folder and before `data/`, with its PNGs in Git LFS (`docs/fleet-plan.md` §5).
- Do remastered sprites (fitted to and coloured from the originals) count as replacements for selling the game, or are they adaptations still under CC BY-NC-SA 3.0? `ASSETS.md` marks them "replaced (…, adaptation)" until decided. (This is not legal advice; it may need one.)
- Should every ship be rendered with one shared camera (tilt and perspective distance; simpler, consistent) or with each original's fitted camera (closer match, more fitting work)? The Human cruiser needed 44° tilt and a perspective camera about 200 units away to fit its frames within 1 %.
- ~~Which modernisation level (A subtle, B balanced, C richest; see `docs/art-direction.md`) becomes the fleet standard?~~ None, 2026-10-18: the variants drifted from the original design; replaced by remastering from the original frames (§6).
- ~~Should new art keep the original ship designs or reinterpret them?~~ Answered 2026-10-18: keep them, the same ship remastered (cleaner and sharper, recognisably the original).
- ~~Team colour on engine lights?~~ Decided 2026-10-18: keep the originals' behaviour. The team tint covers the whole ship, engine lights included (`docs/art-direction.md` §3).
- Minimum supported resolution and graphics hardware? (The logical screen is at least 640 × 480; physical minimum not yet decided.)
- Should the battle view's field of view be capped (for example to a fixed world height and maximum width) so ultrawide windows don't see more of the battlefield than the scenarios were balanced for?
- ~~Should capital ships pick their sprite sheet from UI scale × object scale?~~ Decided 2026-10-18: yes (see §6, sprite sheet decision).
- ~~M2: may Modern add multi-ship selection?~~ Answered 2026-10-20: yes (the Modern goal in §4). It is built as a selection list whose first ship stays the single control object (`docs/modern-controls.md` §2).
- M2: may Modern move the camera away from the flagship (edge or drag scrolling)? The original always centres on it.
- ~~M2: in Modern, do the mouse buttons fire weapons or select and give orders?~~ Answered 2026-10-20 by the Modern goal: they select and give orders; firing stays on keys.
- ~~M2 Modern: should tutorials always run Classic?~~ Decided 2026-10-20: yes, the Modern mode is off in the tutorials (levels with a skip option).
- ~~M2 Modern: A then click for attack-move, or WASD flight?~~ Decided 2026-10-20: flight stays on WASD; attack-move is Ctrl+right-click plus a rebindable key (G).
- ~~M2 Modern: is a pointer to the group enough for a double-tapped group number?~~ Decided 2026-10-20: yes, until the camera work.
- ~~M2 Classic: Left Alt (fire 1) + Tab (warp) and the left Windows key (fire 2) take the game out of focus.~~ Decided 2026-10-20: Classic's fire 1 is X and fire 2 is Z. Saved Classic configs still on the old keys move, unless the new key is taken (`docs/current-state.md` §1.2).
- Should new sprite art keep the original silhouettes exactly? Collision bounds come from the 1× frame sizes, so changing them changes gameplay.
