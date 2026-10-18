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
  - **Modern:** WASD flight, mouse aim and fire, click to select ships and give fleet orders, mouse wheel zoom.
- A rebinding screen so every action can be reassigned to keyboard keys or mouse buttons.
- Bindings are saved in the player's config and survive updates.
- Gamepad support already exists (see `docs/current-state.md` §1.4). Both presets and the rebinding screen must cover gamepad as well as keyboard and mouse.
- Mouse-wheel zoom needs new input handling: the game currently ignores the scroll wheel. M2 includes adding it.
- Exact action list to be taken from `docs/current-state.md`.

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
  - `ASSETS.md` stays unchanged until a real model replaces an original sprite.
- **Finding: the original sprites use a three-quarter view.** They were rendered with a camera tilted about 44° from vertical, viewing from the bottom of the screen, which shortens ships' length on screen. Each ship's pivot also sits above the hull. New renders have to match both, or ships won't line up with their pivots. The angle was fitted for the Human cruiser; other ships give noisier estimates (8°–52°), so the tilt is a per-sprite setting (`--tilt`). Tools: `tools/sprite-render/`, described in `docs/current-state.md` §4.3.

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
    - carrier: IoU 0.94 / 0.92, outline 0.46 px; its p95 is 2.0 px, from frame 0's 1 px antenna.
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
| M4 | Blender pipeline | One ship rendered from a script and working in game (in progress: script done; game loads @2x/@4x sprite sheets; placeholder Human cruiser rendered and tested in game at UI 1×–4× via the override folder; Human cruiser remaster fitted to the original frames and tested in game at UI 1×, 2×, 4× beside original ships; Human fighter and carrier remastered with shared shape kits, awaiting review) |
| M5 | All ships replaced | Every ship type uses new sprites |
| M6 | Effects | New explosions, weapons, thrusters, lighting |

## 10. Open questions

- Which parts of the UI need a full redesign and which only need scaling?
- ~~How many ship types and frames per ship does the game load?~~ Answered 2026-10-18: 82 objects use rotating sprites; 81 have 24 frames (15° steps) and the Bazidan battleship has 36. Frames range from 12 × 11 to 108 × 78 px (see `docs/current-state.md` §4.3).
- Where should finished replacement art live: a fork of the data repository, or a separate Star Bound art folder with a loader change?
- Should every ship be rendered with one shared camera (tilt and perspective distance; simpler, consistent) or with each original's fitted camera (closer match, more fitting work)? The Human cruiser needed 44° tilt and a perspective camera about 200 units away to fit its frames within 1 %.
- ~~Which modernisation level (A subtle, B balanced, C richest; see `docs/art-direction.md`) becomes the fleet standard?~~ None, 2026-10-18: the variants drifted from the original design; replaced by remastering from the original frames (§6).
- ~~Should new art keep the original ship designs or reinterpret them?~~ Answered 2026-10-18: keep them, the same ship remastered (cleaner and sharper, recognisably the original).
- ~~Team colour on engine lights?~~ Decided 2026-10-18: keep the originals' behaviour. The team tint covers the whole ship, engine lights included (`docs/art-direction.md` §3).
- Minimum supported resolution and graphics hardware? (The logical screen is at least 640 × 480; physical minimum not yet decided.)
- Should the battle view's field of view be capped (for example to a fixed world height and maximum width) so ultrawide windows don't see more of the battlefield than the scenarios were balanced for?
- Should capital ships (drawn at 2×, `tpl/capship`) pick their sprite sheet from UI scale × object scale, so the carrier uses its @2x sheet at UI 1×? Today the choice uses the UI scale only (`src/drawing/pix-table.cpp`).
- Should new sprite art keep the original silhouettes exactly? Collision bounds come from the 1× frame sizes, so changing them changes gameplay.
